//
// Copyright (C) 2008-2021 Jonathan Moore Liles (as "Non-Timeline")
// Copyright (C) 2023- Stazed
//
// This file is part of Non-Timeline-XT
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

// generated by Fast Light User Interface Designer (fluid) version 1.0309

#include "Track_Header.H"
#include <FL/Fl_PNG_Image.H>
#include "../../FL/img_io_input_connector_10x10_png.h"
#include "../../FL/img_io_output_connector_10x10_png.h"
static Fl_PNG_Image *output_connector_image = NULL; 
static Fl_PNG_Image *input_connector_image = NULL; 
Track_Header::Track_Header(int X, int Y, int W, int H, const char *L)
  : Fl_Group(X, Y, W, H, L) {
this->box(FL_NO_BOX);
{ Fl_Group* o = box_group = new Fl_Group(0, -1, 200, 67);
  box_group->box(FL_THIN_UP_BOX);
  box_group->color((Fl_Color)48);
  { Fl_Group* o = new Fl_Group(0, -1, 200, 62);
    { name_input = new Fl_Sometimes_Input(15, 2, 140, 22, "input:");
      name_input->box(FL_DOWN_BOX);
      name_input->color(FL_BACKGROUND2_COLOR);
      name_input->selection_color(FL_SELECTION_COLOR);
      name_input->labeltype(FL_NO_LABEL);
      name_input->labelfont(0);
      name_input->labelsize(14);
      name_input->labelcolor(FL_FOREGROUND_COLOR);
      name_input->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
      name_input->when(FL_WHEN_ENTER_KEY);
    } // Fl_Sometimes_Input* name_input
    { track_inputs_indicator = new Fl_Button(0, 27, 29, 16, "in");
      track_inputs_indicator->tooltip("lit if inputs are connected");
      track_inputs_indicator->box(FL_BORDER_BOX);
      track_inputs_indicator->color((Fl_Color)48);
      track_inputs_indicator->selection_color((Fl_Color)90);
      track_inputs_indicator->labelfont(5);
      track_inputs_indicator->labelsize(10);
      track_inputs_indicator->labelcolor((Fl_Color)53);
      track_inputs_indicator->hide();
    } // Fl_Button* track_inputs_indicator
    { track_outputs_indicator = new Fl_Button(31, 27, 24, 16, "out");
      track_outputs_indicator->tooltip("lit if outputs are connected");
      track_outputs_indicator->box(FL_BORDER_BOX);
      track_outputs_indicator->color((Fl_Color)48);
      track_outputs_indicator->selection_color((Fl_Color)90);
      track_outputs_indicator->labelfont(5);
      track_outputs_indicator->labelsize(10);
      track_outputs_indicator->labelcolor((Fl_Color)53);
      track_outputs_indicator->hide();
    } // Fl_Button* track_outputs_indicator
    { menu_button = new Fl_Button(15, 26, 31, 24, "menu");
      menu_button->tooltip("Expand controls");
      menu_button->selection_color((Fl_Color)3);
      menu_button->labelfont(4);
      menu_button->labelsize(10);
    } // Fl_Button* menu_button
    { overlay_controls_button = new Fl_Button(50, 26, 24, 24, "c-");
      overlay_controls_button->tooltip("Expand controls");
      overlay_controls_button->type(1);
      overlay_controls_button->selection_color((Fl_Color)3);
      overlay_controls_button->labelfont(5);
      overlay_controls_button->labelsize(12);
    } // Fl_Button* overlay_controls_button
    { show_all_takes_button = new Fl_Button(77, 26, 24, 24, "t+");
      show_all_takes_button->tooltip("Show all takes");
      show_all_takes_button->type(1);
      show_all_takes_button->selection_color((Fl_Color)3);
      show_all_takes_button->labelfont(5);
      show_all_takes_button->labelsize(12);
    } // Fl_Button* show_all_takes_button
    { rec_button = new Fl_Button(118, 26, 24, 24, "r");
      rec_button->tooltip("arm for recording");
      rec_button->type(1);
      rec_button->selection_color(FL_RED);
      rec_button->labelfont(5);
      rec_button->labelsize(12);
    } // Fl_Button* rec_button
    { mute_button = new Fl_Button(145, 26, 24, 24, "m");
      mute_button->tooltip("mute");
      mute_button->type(1);
      mute_button->selection_color(FL_GREEN);
      mute_button->labelfont(5);
      mute_button->labelsize(12);
    } // Fl_Button* mute_button
    { solo_button = new Fl_Button(172, 26, 24, 24, "s");
      solo_button->tooltip("solo");
      solo_button->type(1);
      solo_button->selection_color((Fl_Color)91);
      solo_button->labelfont(5);
      solo_button->labelsize(12);
    } // Fl_Button* solo_button
    { Fl_Box* o = input_connector_handle = new Fl_Box(157, 4, 18, 18);
      input_connector_handle->tooltip("Drag and drop this input connector to make or break JACK connections");
      input_connector_handle->box(FL_FLAT_BOX);
      o->image( input_connector_image ? input_connector_image : input_connector_image = new Fl_PNG_Image( "input-connector", img_io_input_connector_10x10_png, img_io_input_connector_10x10_png_len ) );
      o->box(FL_NO_BOX);
    } // Fl_Box* input_connector_handle
    { Fl_Box* o = output_connector_handle = new Fl_Box(177, 4, 18, 18);
      output_connector_handle->tooltip("Drag and drop this output connector to make or break JACK connections");
      output_connector_handle->box(FL_FLAT_BOX);
      o->image( output_connector_image ? output_connector_image : output_connector_image =  new Fl_PNG_Image( "output-connector", img_io_output_connector_10x10_png, img_io_output_connector_10x10_png_len ) );
      o->box(FL_NO_BOX);
    } // Fl_Box* output_connector_handle
    o->resizable(0);
    o->end();
  } // Fl_Group* o
  o->resizable(0);
  box_group->end();
} // Fl_Group* box_group
{ Fl_Box* o = new Fl_Box(200, 0, 325, 60, "<Sequence>");
  Fl_Group::current()->resizable(o);
  o->labeltype(FL_NO_LABEL);
} // Fl_Box* o
{ color_box = new Fl_Box(0, 0, 10, 60);
  color_box->box(FL_FLAT_BOX);
  color_box->color((Fl_Color)59);
} // Fl_Box* color_box
end();
}

void Track_Header::draw() {
  color_box->color( color() );
  
  Fl_Group::draw();
}
Control_Sequence_Header::Control_Sequence_Header(int X, int Y, int W, int H, const char *L)
  : Fl_Group(X, Y, W, H, L) {
this->box(FL_NO_BOX);
{ name_input = new Fl_Sometimes_Input(15, 3, 182, 22, "input:");
  name_input->box(FL_DOWN_BOX);
  name_input->color(FL_BACKGROUND2_COLOR);
  name_input->selection_color(FL_SELECTION_COLOR);
  name_input->labeltype(FL_NO_LABEL);
  name_input->labelfont(0);
  name_input->labelsize(14);
  name_input->labelcolor(FL_FOREGROUND_COLOR);
  name_input->textsize(12);
  name_input->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
  name_input->when(FL_WHEN_ENTER_KEY);
} // Fl_Sometimes_Input* name_input
{ menu_button = new Fl_Button(15, 26, 31, 24, "menu");
  menu_button->tooltip("Expand controls");
  menu_button->selection_color((Fl_Color)3);
  menu_button->labelfont(4);
  menu_button->labelsize(10);
} // Fl_Button* menu_button
{ Fl_Blink_Button* o = outputs_indicator = new Fl_Blink_Button(50, 26, 24, 16, "out");
  outputs_indicator->tooltip("lit if outputs are connected");
  outputs_indicator->box(FL_BORDER_BOX);
  outputs_indicator->down_box(FL_BORDER_BOX);
  outputs_indicator->color((Fl_Color)48);
  outputs_indicator->selection_color((Fl_Color)90);
  outputs_indicator->labeltype(FL_NORMAL_LABEL);
  outputs_indicator->labelfont(5);
  outputs_indicator->labelsize(10);
  outputs_indicator->labelcolor(FL_FOREGROUND_COLOR);
  outputs_indicator->align(Fl_Align(FL_ALIGN_CENTER));
  outputs_indicator->when(FL_WHEN_RELEASE);
  o->ignore_input( true );
  o->blink( false );
} // Fl_Blink_Button* outputs_indicator
{ osc_name_output = new Fl_Output(60, 26, 92, 24);
  osc_name_output->color(FL_GRAY0);
  osc_name_output->labeltype(FL_NO_LABEL);
  osc_name_output->textsize(9);
  osc_name_output->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
  osc_name_output->hide();
} // Fl_Output* osc_name_output
{ osc_connect_menu = new Fl_Menu_Button(147, 26, 24, 24);
  osc_connect_menu->color(FL_LIGHT2);
  osc_connect_menu->hide();
} // Fl_Menu_Button* osc_connect_menu
{ delete_button = new Fl_Button(172, 26, 24, 24, "X");
  delete_button->tooltip("Expand controls");
  delete_button->labelfont(5);
  delete_button->labelsize(12);
} // Fl_Button* delete_button
end();
}
Audio_Sequence_Header::Audio_Sequence_Header(int X, int Y, int W, int H, const char *L)
  : Fl_Group(X, Y, W, H, L) {
this->box(FL_NO_BOX);
{ Fl_Group* o = new Fl_Group(0, 0, 200, 55);
  { name_input = new Fl_Sometimes_Input(15, 3, 178, 22, "input:");
    name_input->box(FL_DOWN_BOX);
    name_input->color(FL_BACKGROUND2_COLOR);
    name_input->selection_color(FL_SELECTION_COLOR);
    name_input->labeltype(FL_NO_LABEL);
    name_input->labelfont(0);
    name_input->labelsize(14);
    name_input->labelcolor(FL_FOREGROUND_COLOR);
    name_input->textsize(12);
    name_input->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
    name_input->when(FL_WHEN_ENTER_KEY);
  } // Fl_Sometimes_Input* name_input
  { delete_button = new Fl_Button(167, 26, 24, 24, "X");
    delete_button->tooltip("Expand controls");
    delete_button->labelfont(5);
    delete_button->labelsize(12);
  } // Fl_Button* delete_button
  { promote_button = new Fl_Button(142, 26, 24, 24, "@2");
    promote_button->tooltip("select this take as active sequence");
    promote_button->labelfont(5);
    promote_button->labelsize(12);
  } // Fl_Button* promote_button
  o->resizable(0);
  o->end();
} // Fl_Group* o
end();
resizable(this);
}
