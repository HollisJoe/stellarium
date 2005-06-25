/*
 * Stellarium
 * Copyright (C) 2002 Fabien Ch�reau
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

#include "stel_ui.h"

using namespace s_gui;


Component* stel_ui::createConfigWindow(void)
{
	config_win = new StdBtWin(_("Configuration"));
	config_win->reshape(300,200,400,350);
	config_win->setVisible(core->FlagConfig);

	config_tab_ctr = new TabContainer();
	config_tab_ctr->setSize(config_win->getSize());

	// The current drawing position
	int x,y;
	x=70; y=15;

	// Rendering options
	FilledContainer* tab_render = new FilledContainer();
	tab_render->setSize(config_tab_ctr->getSize());

	s_texture* starp = new s_texture("halo");
	Picture* pstar = new Picture(starp, x-50, y+5, 32, 32);
	tab_render->addComponent(pstar);

	stars_cbx = new LabeledCheckBox(core->FlagStars, _("Stars"));
	stars_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(stars_cbx);
	stars_cbx->setPos(x,y); y+=15;

	star_names_cbx = new LabeledCheckBox(core->FlagStarName, _("Star Names. Up to mag :"));
	star_names_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(star_names_cbx);
	star_names_cbx->setPos(x,y);

	max_mag_star_name = new FloatIncDec(courierFont, tex_up, tex_down, -1.5, 9,
		core->MaxMagStarName, 0.5);
	max_mag_star_name->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(max_mag_star_name);
	max_mag_star_name->setPos(x + 220,y);

	y+=15;

	star_twinkle_cbx = new LabeledCheckBox(core->FlagStarTwinkle, _("Star Twinkle. Amount :"));
	star_twinkle_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(star_twinkle_cbx);
	star_twinkle_cbx->setPos(x,y);

	star_twinkle_amount = new FloatIncDec(courierFont, tex_up, tex_down, 0, 0.6,
		core->StarTwinkleAmount, 0.1);
	star_twinkle_amount->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(star_twinkle_amount);
	star_twinkle_amount->setPos(x + 220,y);

	y+=30;

	s_texture* constellp = new s_texture("bt_constellations");
	Picture* pconstell = new Picture(constellp, x-50, y+5, 32, 32);
	tab_render->addComponent(pconstell);

	constellation_cbx = new LabeledCheckBox(false, _("Constellations"));
	constellation_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(constellation_cbx);
	constellation_cbx->setPos(x,y); y+=15;

	constellation_name_cbx = new LabeledCheckBox(false, _("Constellations Names"));
	constellation_name_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(constellation_name_cbx);
	constellation_name_cbx->setPos(x,y); y+=15;

	sel_constellation_cbx = new LabeledCheckBox(false, _("Selected Constellation Only"));
	sel_constellation_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(sel_constellation_cbx);
	sel_constellation_cbx->setPos(x,y);

	y+=25;

	s_texture* nebp = new s_texture("bt_nebula");
	Picture* pneb = new Picture(nebp, x-50, y, 32, 32);
	tab_render->addComponent(pneb);

	nebulas_cbx = new LabeledCheckBox(false, _("Nebulas"));
	nebulas_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(nebulas_cbx);
	nebulas_cbx->setPos(x,y); y+=15;

	nebulas_names_cbx = new LabeledCheckBox(false, _("Nebulas Names. Up to mag :"));
	nebulas_names_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(nebulas_names_cbx);
	nebulas_names_cbx->setPos(x,y);

	max_mag_nebula_name = new FloatIncDec(courierFont, tex_up, tex_down, 0, 12,
		core->MaxMagNebulaName, 0.5);
	max_mag_nebula_name->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(max_mag_nebula_name);
	max_mag_nebula_name->setPos(x + 220,y);

	y+=30;

	s_texture* planp = new s_texture("bt_planet");
	Picture* pplan = new Picture(planp, x-50, y, 32, 32);
	tab_render->addComponent(pplan);

	planets_cbx = new LabeledCheckBox(false, _("Planets"));
	planets_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(planets_cbx);
	planets_cbx->setPos(x,y);

	moon_x4_cbx = new LabeledCheckBox(false, _("Moon Scale"));
	moon_x4_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(moon_x4_cbx);
	moon_x4_cbx->setPos(x + 150,y);

	y+=15;

	planets_hints_cbx = new LabeledCheckBox(false, _("Planets Hints"));
	planets_hints_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(planets_hints_cbx);
	planets_hints_cbx->setPos(x,y);

	y+=25;

	s_texture* gridp = new s_texture("bt_grid");
	Picture* pgrid = new Picture(gridp, x-50, y, 32, 32);
	tab_render->addComponent(pgrid);

	equator_grid_cbx = new LabeledCheckBox(false, _("Equatorial Grid"));
	equator_grid_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(equator_grid_cbx);
	equator_grid_cbx->setPos(x,y); y+=15;

	azimuth_grid_cbx = new LabeledCheckBox(false, _("Azimuthal Grid"));
	azimuth_grid_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(azimuth_grid_cbx);
	azimuth_grid_cbx->setPos(x,y); y-=15;

	equator_cbx = new LabeledCheckBox(false, _("Equator Line"));
	equator_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(equator_cbx);
	equator_cbx->setPos(x + 150,y); y+=15;

	ecliptic_cbx = new LabeledCheckBox(false, _("Ecliptic Line"));
	ecliptic_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(ecliptic_cbx);
	ecliptic_cbx->setPos(x + 150,y);

	y+=25;

	s_texture* groundp = new s_texture("bt_ground");
	Picture* pground = new Picture(groundp, x-50, y, 32, 32);
	tab_render->addComponent(pground);

	ground_cbx = new LabeledCheckBox(false, _("Ground"));
	ground_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(ground_cbx);
	ground_cbx->setPos(x,y);

	cardinal_cbx = new LabeledCheckBox(false, _("Cardinal Points"));
	cardinal_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(cardinal_cbx);
	cardinal_cbx->setPos(x + 150,y); y+=15;

	atmosphere_cbx = new LabeledCheckBox(false, _("Atmosphere"));
	atmosphere_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(atmosphere_cbx);
	atmosphere_cbx->setPos(x,y);

	fog_cbx = new LabeledCheckBox(false, _("Fog"));
	fog_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateConfigVariables));
	tab_render->addComponent(fog_cbx);
	fog_cbx->setPos(x + 150,y); y+=22;

	LabeledButton* render_save_bt = new LabeledButton(_("Save as default"));
	render_save_bt->setOnPressCallback(callback<void>(this, &stel_ui::saveRenderOptions));
	tab_render->addComponent(render_save_bt);
	render_save_bt->setPos(x + 50,y);
	render_save_bt->setSize(170,25); y+=20;

	// Date & Time options
	FilledContainer* tab_time = new FilledContainer();
	tab_time->setSize(config_tab_ctr->getSize());

	x=10; y=10;

	Label* tclbl = new Label(_("\1 Current Time :"));
	tclbl->setPos(x,y); y+=20;
	tab_time->addComponent(tclbl);

	time_current = new Time_item(courierFont, tex_up, tex_down);
	time_current->setOnChangeTimeCallback(callback<void>(this, &stel_ui::setCurrentTimeFromConfig));
	tab_time->addComponent(time_current);
	time_current->setPos(50,y); y+=80;

	Label* tzbl = new Label(_("\1 Time Zone :"));
	tzbl->setPos(x,y); y+=20;
	tab_time->addComponent(tzbl);

	Label* system_tz_lbl = new Label(_("\1 Using System Default Time Zone"));
	tab_time->addComponent(system_tz_lbl);
	system_tz_lbl->setPos(50 ,y); y+=20;
	string tmpl("(" + core->observatory->get_time_zone_name_from_system(core->navigation->get_JDay()) + ")");
	
	for (unsigned int i=0;i<tmpl.size();i++) {if ((unsigned int)tmpl[i]<0 || (unsigned int)tmpl[i]>255) tmpl[i]='*';}
	system_tz_lbl2 = new Label(tmpl);
	tab_time->addComponent(system_tz_lbl2);
	
	system_tz_lbl2->setPos(70 ,y); y+=30;

	Label* time_speed_lbl = new Label(_("\1 Time speed : "));
	tab_time->addComponent(time_speed_lbl);
	time_speed_lbl->setPos(x ,y); y+=20;

	time_speed_lbl2 = new Label("\1 Current Time Speed is XX sec/sec.");
	tab_time->addComponent(time_speed_lbl2);
	time_speed_lbl2->setPos(50 ,y); y+=30;

	TextLabel* ts_lbl = new TextLabel(_("Use key J and L to decrease and increase\n   time speed.\nUse key K to return to real time speed."));
	tab_time->addComponent(ts_lbl);
	ts_lbl->setPos(50 ,y); y+=30;

	// Location options
	FilledContainer* tab_location = new FilledContainer();
	tab_location->setSize(config_tab_ctr->getSize());

	x=5; y=5;
	s_texture * earth = new s_texture("earthmap");
	s_texture * pointertex = new s_texture("neb");
	earth_map = new MapPicture(earth, pointertex, x, y, tab_location->getSizex()-10, 250);
	earth_map->setOnPressCallback(callback<void>(this, &stel_ui::setObserverPositionFromMap));
	tab_location->addComponent(earth_map);
	y+=earth_map->getSizey() + 20;

	Label * lbllong = new Label(_("Longitude : "));
	lbllong->setPos(30, y+1);
	Label * lbllat = new Label(_("Latitude : "));
	lbllat->setPos(30, y+21);
	tab_location->addComponent(lbllong);
	tab_location->addComponent(lbllat);

	long_incdec	= new FloatIncDec(courierFont, tex_up, tex_down, -180, 180, 0, 0.05);
	long_incdec->setSizex(100);
	long_incdec->setOnPressCallback(callback<void>(this, &stel_ui::setObserverPositionFromIncDec));
	long_incdec->setPos(110,y);
	lat_incdec	= new FloatIncDec(courierFont, tex_up, tex_down, -90, 90, 0, 0.05);
	lat_incdec->setSizex(100);
	lat_incdec->setOnPressCallback(callback<void>(this, &stel_ui::setObserverPositionFromIncDec));
	lat_incdec->setPos(110,y+20);
	tab_location->addComponent(long_incdec);
	tab_location->addComponent(lat_incdec);

	LabeledButton* location_save_bt = new LabeledButton(_("Save location"));
	location_save_bt->setOnPressCallback(callback<void>(this, &stel_ui::saveObserverPosition));
	location_save_bt->setPos(200,y+5);
	location_save_bt->setSize(170,25);
	tab_location->addComponent(location_save_bt);

	// Video Options
	FilledContainer* tab_video = new FilledContainer();
	tab_video->setSize(config_tab_ctr->getSize());

	x=10; y=10;
	Label * lblvideo1 = new Label(_("\1 Projection :"));
	lblvideo1->setPos(x, y);
	tab_video->addComponent(lblvideo1);

	x=50; y+=20;

	fisheye_projection_cbx = new LabeledCheckBox(core->projection->get_type()==FISHEYE_PROJECTOR, _("Fisheye Projection Mode"));
	fisheye_projection_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateVideoVariables));
	tab_video->addComponent(fisheye_projection_cbx);
	fisheye_projection_cbx->setPos(x,y); y+=15;

	disk_viewport_cbx = new LabeledCheckBox(core->projection->get_viewport_type()==DISK, _("Disk Viewport"));
	disk_viewport_cbx->setOnPressCallback(callback<void>(this, &stel_ui::updateVideoVariables));
	tab_video->addComponent(disk_viewport_cbx);
	disk_viewport_cbx->setPos(x,y); y+=35;

	Label * lblvideo2 = new Label(_("\1 Screen Resolution :"));
	lblvideo2->setPos(10, y);
	tab_video->addComponent(lblvideo2); y+=20;

	Label * lblvideo3 = new Label(_("Restart program for"));
	Label * lblvideo4 = new Label(_("change to apply."));
	lblvideo3->setPos(200, y+25);
	lblvideo4->setPos(200, y+40);
	tab_video->addComponent(lblvideo3);
	tab_video->addComponent(lblvideo4);

	screen_size_sl = new StringList();
	screen_size_sl->setPos(x,y);
	screen_size_sl->addItem("640x480");
	screen_size_sl->addItem("800x600");
	screen_size_sl->addItem("1024x768");
	screen_size_sl->addItem("1280x1024");
	screen_size_sl->addItem("1600x1200");
	screen_size_sl->adjustSize();
	char vs[100];
	sprintf(vs, "%dx%d", core->screen_W, core->screen_H);
	screen_size_sl->setValue(vs);
	tab_video->addComponent(screen_size_sl);

	y+=100;

	LabeledButton* video_save_bt = new LabeledButton(_("Save as default"));
	video_save_bt->setOnPressCallback(callback<void>(this, &stel_ui::setVideoOption));
	tab_video->addComponent(video_save_bt);
	video_save_bt->setPos(x + 50,y);
	video_save_bt->setSize(170,25); y+=20;

	config_tab_ctr->setTexture(flipBaseTex);
	config_tab_ctr->addTab(tab_time, _("Date & Time"));
	config_tab_ctr->addTab(tab_location, _("Location"));
	config_tab_ctr->addTab(tab_video, _("Video"));
	config_tab_ctr->addTab(tab_render, _("Rendering"));
	config_win->addComponent(config_tab_ctr);
	config_win->setOnHideBtCallback(callback<void>(this, &stel_ui::config_win_hideBtCallback));

	
	return config_win;
}

void stel_ui::updateConfigVariables(void)
{
	core->commander->execute_command("flag stars ", stars_cbx->getState());
	core->commander->execute_command("flag star_name ", star_names_cbx->getState());
	core->commander->execute_command("set max_mag_star_name ", max_mag_star_name->getValue());
	core->commander->execute_command("flag star_twinkle ", star_twinkle_cbx->getState());
	core->commander->execute_command("set star_twinkle_amount ", star_twinkle_amount->getValue());
	core->commander->execute_command("flag constellation_drawing ", constellation_cbx->getState());
	core->commander->execute_command("flag constellation_name ", constellation_name_cbx->getState());
	core->commander->execute_command("flag constellation_pick ", sel_constellation_cbx->getState());
	core->commander->execute_command("flag nebula ", nebulas_cbx->getState());
	core->commander->execute_command("flag nebula_name ", nebulas_names_cbx->getState());
	core->commander->execute_command("set max_mag_nebula_name ", max_mag_nebula_name->getValue());
	core->commander->execute_command("flag planets ", planets_cbx->getState());
	core->commander->execute_command("flag planets_hints ", planets_hints_cbx->getState());
	core->commander->execute_command("set moon_scale ", moon_x4_cbx->getState() ? core->MoonScale : 1.f);
	core->commander->execute_command("flag equatorial_grid ", equator_grid_cbx->getState());
	core->commander->execute_command("flag azimuthal_grid ", azimuth_grid_cbx->getState());
	core->commander->execute_command("flag equator_line ", equator_cbx->getState());
	core->commander->execute_command("flag ecliptic_line ", ecliptic_cbx->getState());
	core->commander->execute_command("flag landscape ", ground_cbx->getState());
	core->commander->execute_command("flag cardinal_points ", cardinal_cbx->getState());
	core->commander->execute_command("flag atmosphere ", atmosphere_cbx->getState());
	core->commander->execute_command("flag fog ", fog_cbx->getState());
}

void stel_ui::setCurrentTimeFromConfig(void)
{
	//	core->navigation->set_JDay(time_current->getJDay() - core->observatory->get_GMT_shift()*JD_HOUR);
	core->commander->execute_command(string("date local " + time_current->getDateString()));
}

void stel_ui::setObserverPositionFromMap(void)
{
	std::ostringstream oss;
	oss << "moveto lat " << earth_map->getPointerLatitude() << " lon " << earth_map->getPointerLongitude();
	core->commander->execute_command(oss.str());
}

void stel_ui::setObserverPositionFromIncDec(void)
{
	std::ostringstream oss;
	oss << "moveto lat " << lat_incdec->getValue() << " lon " << long_incdec->getValue();
	core->commander->execute_command(oss.str());
}

void stel_ui::saveObserverPosition(void)
{
	core->observatory->save(core->ConfigDir + core->config_file, "init_location");
}

void stel_ui::saveRenderOptions(void)
{
	cout << _("Saving rendering options in file ") << core->ConfigDir + core->config_file << endl;

	init_parser conf;
	conf.load(core->ConfigDir + core->config_file);

	conf.set_boolean("astro:flag_stars", core->FlagStars);
	conf.set_boolean("astro:flag_star_name", core->FlagStarName);
	conf.set_double("stars:max_mag_star_name", core->MaxMagStarName);
	conf.set_boolean("stars:flag_star_twinkle", core->FlagStarTwinkle);
	conf.set_double("stars:star_twinkle_amount", core->StarTwinkleAmount);
	conf.set_boolean("viewing:flag_constellation_drawing", core->constellation_get_flag_lines());
	conf.set_boolean("viewing:flag_constellation_name", core->constellation_get_flag_names());
	conf.set_boolean("viewing:flag_constellation_pick", core->FlagConstellationPick);
	conf.set_boolean("astro:flag_nebula", core->FlagNebula);
	conf.set_boolean("astro:flag_nebula_name", core->FlagNebulaName);
	conf.set_double("astro:max_mag_nebula_name", core->MaxMagNebulaName);
	conf.set_boolean("astro:flag_planets", core->FlagPlanets);
	conf.set_boolean("astro:flag_planets_hints", core->FlagPlanetsHints);
	conf.set_double("viewing:moon_scale", core->ssystem->get_moon()->get_sphere_scale());
	conf.set_boolean("viewing:flag_equatorial_grid", core->FlagEquatorialGrid);
	conf.set_boolean("viewing:flag_azimutal_grid", core->FlagAzimutalGrid);
	conf.set_boolean("viewing:flag_equator_line", core->FlagEquatorLine);
	conf.set_boolean("viewing:flag_ecliptic_line", core->FlagEclipticLine);
	conf.set_boolean("landscape:flag_ground", core->FlagLandscape);
	conf.set_boolean("viewing:flag_cardinal_points", core->FlagCardinalPoints);
	conf.set_boolean("landscape:flag_atmosphere", core->FlagAtmosphere);
	conf.set_boolean("landscape:flag_fog", core->FlagFog);

	conf.save(core->ConfigDir + core->config_file);
}

void stel_ui::setTimeZone(void)
{
	core->observatory->set_custom_tz_name(tzselector->gettz());
}

void stel_ui::setVideoOption(void)
{
	string s = screen_size_sl->getValue();
	int i = s.find("x");
	int w = atoi(s.substr(0,i).c_str());
	int h = atoi(s.substr(i+1,s.size()).c_str());

	cout << _("Saving video size ") << w << "x" << h << _(" in file ") << core->ConfigDir + core->config_file << endl;

	init_parser conf;
	conf.load(core->ConfigDir + core->config_file);

	switch (core->projection->get_type())
	{
		case FISHEYE_PROJECTOR : conf.set_str("projection:type", "fisheye"); break;
		case PERSPECTIVE_PROJECTOR :
		default :
			conf.set_str("projection:type", "perspective"); break;
	}

	switch (core->projection->get_viewport_type())
	{
		case SQUARE : conf.set_str("projection:viewport", "square"); break;
		case DISK : conf.set_str("projection:viewport", "disk"); break;
		case MAXIMIZED :
		default :
			conf.set_str("projection:viewport", "maximized"); break;
	}

	conf.set_int("video:screen_w", w);
	conf.set_int("video:screen_h", h);
	conf.save(core->ConfigDir + core->config_file);
}

void stel_ui::updateVideoVariables(void)
{
	if (fisheye_projection_cbx->getState() && core->projection->get_type()!=FISHEYE_PROJECTOR)
	{
		// Switch to fisheye projection
		Fisheye_projector* p = new Fisheye_projector(*(core->projection));
		delete core->projection;
		core->projection = p;
	}
	if (!fisheye_projection_cbx->getState() && core->projection->get_type()==FISHEYE_PROJECTOR)
	{
		// Switch to perspective projection
		Projector* p = new Projector(*(core->projection));
		delete core->projection;
		core->projection = p;
		core->projection->set_minmaxfov(0.001, 100.);
	}


	if (disk_viewport_cbx->getState() && core->projection->get_viewport_type()!=DISK)
	{
		core->projection->set_disk_viewport();
	}
	if (!disk_viewport_cbx->getState() && core->projection->get_viewport_type()==DISK)
	{
		core->projection->maximize_viewport();
	}

}

void stel_ui::updateConfigForm(void)
{
	stars_cbx->setState(core->FlagStars);
	star_names_cbx->setState(core->FlagStarName);
	max_mag_star_name->setValue(core->MaxMagStarName);
	star_twinkle_cbx->setState(core->FlagStarTwinkle);
	star_twinkle_amount->setValue(core->StarTwinkleAmount);
	constellation_cbx->setState(core->constellation_get_flag_lines());
	constellation_name_cbx->setState(core->constellation_get_flag_names());
	sel_constellation_cbx->setState(core->FlagConstellationPick);
	nebulas_cbx->setState(core->FlagNebula);
	nebulas_names_cbx->setState(core->FlagNebulaName);
	max_mag_nebula_name->setValue(core->MaxMagNebulaName);
	planets_cbx->setState(core->FlagPlanets);
	planets_hints_cbx->setState(core->FlagPlanetsHints);
	moon_x4_cbx->setState(core->ssystem->get_moon()->get_sphere_scale()!=1.f);
	equator_grid_cbx->setState(core->FlagEquatorialGrid);
	azimuth_grid_cbx->setState(core->FlagAzimutalGrid);
	equator_cbx->setState(core->FlagEquatorLine);
	ecliptic_cbx->setState(core->FlagEclipticLine);
	ground_cbx->setState(core->FlagLandscape);
	cardinal_cbx->setState(core->FlagCardinalPoints);
	atmosphere_cbx->setState(core->FlagAtmosphere);
	fog_cbx->setState(core->FlagFog);

	earth_map->setPointerLongitude(core->observatory->get_longitude());
	earth_map->setPointerLatitude(core->observatory->get_latitude());
	long_incdec->setValue(core->observatory->get_longitude());
	lat_incdec->setValue(core->observatory->get_latitude());

	time_current->setJDay(core->navigation->get_JDay() + core->observatory->get_GMT_shift(core->navigation->get_JDay())*JD_HOUR);
	system_tz_lbl2->setLabel("(" +
		 core->observatory->get_time_zone_name_from_system(core->navigation->get_JDay()) + ")");

	static char tempstr[100];
	sprintf(tempstr, "%.1f", core->navigation->get_time_speed()/JD_SECOND);
	time_speed_lbl2->setLabel("\1 Current Time Speed is x" + string(tempstr));

	fisheye_projection_cbx->setState(core->projection->get_type()==FISHEYE_PROJECTOR);
	disk_viewport_cbx->setState(core->projection->get_viewport_type()==DISK);
}

void stel_ui::config_win_hideBtCallback(void)
{
	core->FlagConfig = false;
	config_win->setVisible(false);
	bt_flag_config->setState(0);
}
