__kernel void test (int nL, int nW,
		__global float4* pxlcol,	
		__global float4* pxlpos,
		__global float4* ltpos,	
		__global float4* ltcol,
		__global float4* wp1,
		__global float4* wp2,
		float4 pxltrans,
		float4 amb,
		float mbc,
		__global float4* lim,
		int N,
		__global bool* liu,
		int framc,
		int pxlr, int pxlc)
{
	
	int i = get_global_id(0);								// get global memory ID
	
	//if (i % 2 == framc || i % 1 == framc)		// alternate pixels on alternate frames
	{
		float4 oldc = pxlcol[i];							// stores old pixel colour
		float4 pp = pxlpos[i] + pxltrans;			// stores pixel position

		float4 newc = { 0.0f, 0.0f, 0.0f, 0.0f };	// temporary colour

		float dcy;													// distance of first light source to ith pixel

		// finding shadows
		for (int L = 0; L < nL; L++)						// for each light
		{
			if (liu[L*N + i])									// for each pixel colour of Lth light
			{
				liu[L*N + i] = false; //L == 0 ? true : false;						// diactivates this pixel's update
				float4 lp = ltpos[L] + pxltrans;		// light's position
				float4 a_b = pp - lp;						// vector from light to ith pixel
				float d2 = a_b.x*a_b.x + a_b.y*a_b.y + 0.00001f;	// distance sq
				if (L = 0) dcy = sqrt(d2);					// asign dcy
				bool inshadow = false;					// this is to find whether this pixel needs to add RGB from this light

				for (int W = 0; W < nW; W++)		// for each wall
				{
					float4 p1 = wp1[W] + pxltrans;	// point 1
					float4 p2 = wp2[W] + pxltrans;	// point 2
					float4 _1_2 = p2 - p1;					// vector of the wall
					float4 _1_l = lp - p1;						// vector of point 1 to light

					// this is testing to see whether the wall intersects the light ray
					float s = (a_b.x * _1_l.y - a_b.y * _1_l.x) / ( a_b.x * _1_2.y - _1_2.x * a_b.y );
					if (s >= 0.0f && s <= 1.0f)
					{
						float t =  (_1_2.x * _1_l.y - _1_2.y * _1_l.x) / ( a_b.x * _1_2.y - _1_2.x * a_b.y );
						if (t >= 0.0f && t <= 1.0f)
						{
							inshadow = true;				
							break;	// only needs one wall to determine a shadow for this light			
						}	
					}

				}

				if (!inshadow)						// if not in shadow
				{
					float4 lc = ltcol[L];			// light colour
					float rt = 1.0f / (4.0f * 3.141f *sqrt(d2));	// intensity factor
					float srt =  rt * lc.w;		// second intensity factor including brightness of light
					//lim[L*N + i] =  lc * srt ;	// asign this light's pixel version 	
					//newc +=  lc * srt;
					
					float4 tempc = lc * srt;

					// pixel averaging for blur effect
					float4 avg = { 0.0f,0.0f,0.0f,0.0f };
					avg += tempc;				
					avg += lim[L*N + i - 1];			
					avg += lim[L*N + i + 1];
					avg += lim[L*N + i - pxlc];
					avg += lim[L*N + i + pxlc];
					avg += pxlcol[L*N + i  - 2];			
					avg += pxlcol[L*N + i  + 2];
					avg += pxlcol[L*N + i  - pxlc*2];
					avg += pxlcol[L*N + i  + pxlc*2];
					avg += pxlcol[L*N + i  - 3];			
					avg += pxlcol[L*N + i  + 3];
					avg += pxlcol[L*N + i  - pxlc*3];
					avg += pxlcol[L*N + i  + pxlc*3];
					avg /= 13.0f;
					float4 da = avg - tempc;

					lim[L*N + i] = tempc + da/(1+pow(lc.w/2/d2,8));
				}
				else										// if in shadow
				{
					lim[L*N + i].x = 0.0f;		// no colour
					lim[L*N + i].y = 0.0f;
					lim[L*N + i].z = 0.0f;
					lim[L*N + i].w = 0.0f;
				}
			}
			newc += lim[L*N + i];			// accumerlate RGB values
		
		}
		newc /= nL*2;
		// giving the walls a seperate colour
		int wc = 0;
		for (int W = 0; W < nW; W++)			// for each wall
		{
			float4 p1 = wp1[W] + pxltrans;	// point 1
			float4 p2 = wp2[W] + pxltrans;	// point 2
			float4 _1_2 = p2 - p1;					// vector of wall
			float m_1_2 = sqrt(_1_2.x*_1_2.x + _1_2.y*_1_2.y);	// mag of wall vector
			float4 p_1 = p1 - pp;						// vector of pixel to point 1
			float d = fabs((_1_2.x*p_1.y - p_1.x*_1_2.y)/m_1_2);	// closest perpedicular distance of pixel to wall
			if (d < (pxltrans.x) *2.0f*1.0f)			// if closer than
			{
				float4 c_a = pp - p1;					// vector of point 1 to pixel
				float4 n = _1_2 / m_1_2;			// normalised vector of wall
				float s  = (c_a.x*n.x + c_a.y*n.y) / m_1_2;	// projection factor of pixel on wall
				if (s >= 0 && s <= 1)					// if within the line
				{
					newc *= 2;								// make colour brighter
					wc++;
				}
			}
		}
		if (wc > 0) newc /= wc;						// average to stop the multipling effect of overlapping walls

		float bleed = (newc.x + newc.y + newc.z) / 3.0f / 5.0f;	// bleed colour
	
		newc.x += bleed;
		newc.y += bleed;
		newc.z += bleed;

		newc += amb;	// ambience

		// clamp to 1.0f
		if (newc.x > 1.0f) newc.x = 1.0f;
		if (newc.y > 1.0f) newc.y = 1.0f;
		if (newc.z > 1.0f) newc.z = 1.0f;

		float tb = mbc;
		float4 dc = newc - oldc;		// motion blur difference
		//if ((dc.x+dc.y+dc.z)/3 > 0.5f) tb *= 10;
		pxlcol[i] = oldc + dc / tb;	

		// pixel averaging for blur effect
		float4 avg = { 0.0f,0.0f,0.0f,0.0f };
		avg += pxlcol[i];
		avg += pxlcol[i - 1];			
		avg += pxlcol[i + 1];
		avg += pxlcol[i - 144];
		avg += pxlcol[i + 144];
		avg /= 5.0f;

		float4 da = avg - newc;
		//pxlcol[i] = newc + da/100;
	}
}

__kernel void test2 (int nL, int nW,
		__global float4* pxlcol,	
		__global float4* pxlpos,
		__global float4* ltpos,	
		__global float4* ltcol,
		__global float4* wp1,
		__global float4* wp2,
		float4 pxltrans,
		float4 amb,
		float mbc,
		int N, int pxlr, int pxlc)
{
	
	int i = get_global_id(0);								// get global memory ID
	
		float4 oldc = pxlcol[i];							// stores old pixel colour
		float4 pp = pxlpos[i] + pxltrans;			// stores pixel position

		float4 newc = { 0.0f, 0.0f, 0.0f, 0.0f };	// temporary colour
		float4 newcl = { 0.0f, 0.0f, 0.0f, 0.0f };	// temporary colour
		float dcy;													// distance of first light source to ith pixel
		// finding shadows
		for (int L = 0; L < nL; L++)						// for each light
		{
			float4 lp = ltpos[L] + pxltrans;		// light's position
			float4 a_b = pp - lp;						// vector from light to ith pixel
			float d2 = a_b.x*a_b.x + a_b.y*a_b.y + 0.00001f;	// distance sq
			bool inshadow = false;					// this is to find whether this pixel needs to add RGB from this light
			if (L == 0) dcy = sqrt(d2);
			for (int W = 0; W < nW; W++)		// for each wall
			{
				float4 p1 = wp1[W] + pxltrans;	// point 1
				float4 p2 = wp2[W] + pxltrans;	// point 2
				float4 _1_2 = p2 - p1;					// vector of the wall
				float4 _1_l = lp - p1;						// vector of point 1 to light

				// this is testing to see whether the wall intersects the light ray
				// both ratios can create a line that can be used to find what area of pixels are in shadow
				// this is the ratio along the vector of the wall, if light ray is between each point (if pixel is in the path of a wall)
				float s = (a_b.x * _1_l.y - a_b.y * _1_l.x) / ( a_b.x * _1_2.y - _1_2.x * a_b.y ); 
				if (s >= 0.0f && s <= 1.0f)
				{
					// this is the ratio of the wall along the light ray, if wall is within the light ray (if wall blocks the pixel)
					float t =  (_1_2.x * _1_l.y - _1_2.y * _1_l.x) / ( a_b.x * _1_2.y - _1_2.x * a_b.y ); 
					if (t >= 0.0f && t <= 1.0f)
					{
						inshadow = true;				
						break;	// only needs one wall to determine a shadow for this light			
					}	
				}

			}

			if (!inshadow)						// if not in shadow
			{
				float4 lc = ltcol[L];			// light colour
				float rt = 1.0f / (4.0f * 3.141f *sqrt(d2));	// intensity factor
				float srt =  rt * lc.w;		// second intensity factor including brightness of light
				float4 tempc = lc * srt;

				// pixel averaging for blur effect
				float4 avg = { 0.0f,0.0f,0.0f,0.0f };
				avg += tempc;				
				avg += pxlcol[i - 1];			
				avg += pxlcol[i + 1];
				avg += pxlcol[i - pxlc];
				avg += pxlcol[i + pxlc];

				avg += pxlcol[i - 2];			
				avg += pxlcol[i + 2];
				avg += pxlcol[i - pxlc*2];
				avg += pxlcol[i + pxlc*2];
				avg += pxlcol[i - 3];			
				avg += pxlcol[i + 3];
				avg += pxlcol[i - pxlc*3];
				avg += pxlcol[i + pxlc*3];

				avg /= 13.0f;
				float4 da = avg - tempc;

				//barrier(CLK_LOCAL_MEM_FENCE); // Wait for others in work-group

				newc += tempc;// + da /(1+pow(lc.w/4/(d2),8));
				//newcl += da /(1+pow(lc.w/4/(d2),8));
			}
		}
		
		//float4 da2 = newcl - newc;
		//newc += da2;
		newc /= nL*2;

		// giving the walls a seperate colour
		bool awall = false;
		int wc = 0;
		for (int W = 0; W < nW; W++)			// for each wall
		{
			float4 p1 = wp1[W] + pxltrans;	// point 1
			float4 p2 = wp2[W] + pxltrans;	// point 2
			float4 _1_2 = p2 - p1;					// vector of wall
			float m_1_2 = sqrt(_1_2.x*_1_2.x + _1_2.y*_1_2.y);	// mag of wall vector
			float4 p_1 = p1 - pp;						// vector of pixel to point 1
			float d = fabs((_1_2.x*p_1.y - p_1.x*_1_2.y)/m_1_2);	// closest perpedicular distance of pixel to wall
			if (d < (pxltrans.x) *2.0f*1.0f)			// if closer than
			{
				float4 c_a = pp - p1;					// vector of point 1 to pixel
				float4 n = _1_2 / m_1_2;			// normalised vector of wall
				float s  = (c_a.x*n.x + c_a.y*n.y) / m_1_2;	// projection factor of pixel on wall
				if (s >= 0 && s <= 1)					// if within the line
				{
					//newc *= 2;								// make colour brighter
					wc++;
					awall = true;
				}
			}
		}
		if (wc > 0) newc /= wc;						// average to stop the multipling effect of overlapping walls

		float bleed = (newc.x + newc.y + newc.z) / 3.0f / 5.0f;	// bleed colour
	
		newc.x += bleed;
		newc.y += bleed;
		newc.z += bleed;

		newc += amb;	// ambience
		
		// clamp to 1.0f
		if (newc.x > 1.0f) newc.x = 1.0f;
		if (newc.y > 1.0f) newc.y = 1.0f;
		if (newc.z > 1.0f) newc.z = 1.0f;

		float tb = mbc;
		float4 dc = newc - oldc;		// motion blur difference
		pxlcol[i] = oldc + dc / tb;	

		if (!awall && false)
		{
			// pixel averaging for blur effect
			float4 avg = { 0.0f,0.0f,0.0f,0.0f };
			avg += pxlcol[i];
			avg += pxlcol[i - 1];			
			avg += pxlcol[i + 1];
			avg += pxlcol[i - pxlc];
			avg += pxlcol[i + pxlc];

			avg += pxlcol[i - 2];			
			avg += pxlcol[i + 2];
			avg += pxlcol[i - pxlc*2];
			avg += pxlcol[i + pxlc*2];
			avg += pxlcol[i - 3];			
			avg += pxlcol[i + 3];
			avg += pxlcol[i - pxlc*3];
			avg += pxlcol[i + pxlc*3];

			avg /= 13.0f;

			float4 da = avg - newc;
			pxlcol[i] = newc + da/(1+pow(ltcol[0].w/3/(dcy),8));
		}
}