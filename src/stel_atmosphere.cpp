/*
 * Stellarium
 * Copyright (C) 2003 Fabien Ch�reau
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

//	Class which compute and display the daylight sky color using openGL
//	the sky is computed with the skylight class.

// TODO : Adaptative resolution for optimization

#include "stellarium.h"
#include "stel_utility.h"
#include "stellastro.h"
#include "stel_atmosphere.h"

stel_atmosphere::stel_atmosphere() : sky_resolution(48), tab_sky(NULL), atm_on(0), atm_intensity(0)
{
	// Create the vector array used to store the sky color on the full field of view
	tab_sky = new Vec3f*[sky_resolution+1];
	for (int k=0; k<sky_resolution+1 ;k++)
	{
		tab_sky[k] = new Vec3f[sky_resolution+1];
	}
}

stel_atmosphere::~stel_atmosphere()
{
	for (int k=0; k<sky_resolution+1 ;k++)
	{
		if (tab_sky[k]) delete tab_sky[k];
	}
	if (tab_sky) delete tab_sky;
}

void stel_atmosphere::show_atmosphere(void)
{
  atm_on = 1;
}

void stel_atmosphere::hide_atmosphere(void)
{
  atm_on = 0;
}

float stel_atmosphere::get_intensity(void)
{
  return atm_intensity;
}

void stel_atmosphere::compute_color(double JD,  int delta_time, Vec3d sunPos, Vec3d moonPos, float moon_phase,
	tone_reproductor * eye, Projector* prj,
	float latitude, float altitude, float temperature, float relative_humidity)
{

	float delta_intensity = delta_time/3000.f;
	// update fade
	if(!atm_on) {
	  if( ai > delta_intensity ) {ai -= delta_intensity;} 
	  else {ai = 0;}
	} else {
	  if( ai + delta_intensity <= 1 ) {ai += delta_intensity;} 
	  else {ai = 1;}
	}

	atm_intensity = ai*ai;

	// no need to calculate if not visible
	if( atm_intensity < .001)
	{
		eye->set_world_adaptation_luminance(3.75f);
		return;
	}

	//Vec3d obj;
	skylight_struct2 b2;

	sunPos.normalize();
	moonPos.normalize();

	float sun_pos[3];
	sun_pos[0] = sunPos[0];
	sun_pos[1] = sunPos[1];
	sun_pos[2] = sunPos[2];

	float moon_pos[3];
	moon_pos[0] = moonPos[0];
	moon_pos[1] = moonPos[1];
	moon_pos[2] = moonPos[2];

	sky.set_paramsv(sun_pos, 5.f);

	skyb.set_loc(latitude * M_PI/180., altitude, temperature, relative_humidity);
	skyb.set_sun_moon(moon_pos[2], sun_pos[2]);

	// Calculate the date from the julian day.
	ln_date date;
	get_date(JD, &date);

	skyb.set_date(date.years, date.months, moon_phase);

	float stepX = (float)prj->viewW() / sky_resolution;
	float stepY = (float)prj->viewH() / sky_resolution;
	float viewport_left = (float)prj->view_left();
	float viewport_bottom = (float)prj->view_bottom();

	Vec3d point(1., 0., 0.);

	// Variables used to compute the average sky luminance
	double sum_lum = 0.;
	unsigned int nb_lum = 0;

	// Compute the sky color for every point above the ground
	for (int x=0; x<=sky_resolution; ++x)
	{
		for(int y=0; y<=sky_resolution; ++y)
		{
			prj->unproject_local((double)viewport_left+x*stepX, (double)viewport_bottom+y*stepY,point);
			point.normalize();

			if (point[2]<=0)
			{
				point[2] = -point[2];
				// The sky below the ground is the symetric of the one above :
				// it looks nice and gives proper values for brightness estimation
			}

			b2.pos[0] = point[0]; b2.pos[1] = point[1]; b2.pos[2] = point[2];

			// Use the skylight model for the color
			sky.get_xyY_valuev(&b2);

			// Use the skybright.cpp 's models for brightness which gives better results.
			b2.color[2] = skyb.get_luminance(moon_pos[0]*b2.pos[0]+moon_pos[1]*b2.pos[1]+
					moon_pos[2]*b2.pos[2] - 0.0000001, sun_pos[0]*b2.pos[0]+sun_pos[1]*b2.pos[1]+
					sun_pos[2]*b2.pos[2] - 0.0000001, b2.pos[2]);


			sum_lum+=b2.color[2];
			++nb_lum;
			eye->xyY_to_RGB(b2.color);
			tab_sky[x][y].set(b2.color[0],b2.color[1],b2.color[2]);
		}
	}

	//	printf("luminance %f\t", 3.75f + 3.5*sum_lum/nb_lum*atm_intensity );
	eye->set_world_adaptation_luminance(3.75f + 3.5*sum_lum/nb_lum*atm_intensity );
	sum_lum = 0.f;
	nb_lum = 0;
}



// Draw the atmosphere using the precalc values stored in tab_sky
void stel_atmosphere::draw(Projector* prj, int delta_time)
{

	if(atm_intensity > 0 ) {

	  // printf("Atm int: %f\n", atm_intensity);
	  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);

	  float stepX = (float)prj->viewW() / sky_resolution;
	  float stepY = (float)prj->viewH() / sky_resolution;
	  float viewport_left = (float)prj->view_left();
	  float view_bottom = (float)prj->view_bottom();

	  glDisable(GL_TEXTURE_2D);
	  glEnable(GL_BLEND);
	  prj->set_orthographic_projection();	// set 2D coordinate
	  for (int y2=0; y2<sky_resolution; ++y2)
	    {
	      glBegin(GL_TRIANGLE_STRIP);
	      for(int x2=0; x2<sky_resolution+1; ++x2)
		{
		  glColor3f(atm_intensity*tab_sky[x2][y2][0],atm_intensity*tab_sky[x2][y2][1],
			    atm_intensity*tab_sky[x2][y2][2]);
		  glVertex2i((int)(viewport_left+x2*stepX),(int)(view_bottom+y2*stepY));
		  glColor3f(atm_intensity*tab_sky[x2][y2+1][0],atm_intensity*tab_sky[x2][y2+1][1],
			    atm_intensity*tab_sky[x2][y2+1][2]);
		  glVertex2i((int)(viewport_left+x2*stepX),(int)(view_bottom+(y2+1)*stepY));
		}
	      glEnd();
	    }
	  prj->reset_perspective_projection();
	}


}
