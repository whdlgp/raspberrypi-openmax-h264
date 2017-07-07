#ifndef CAMERA_H
#define CAMERA_H

#include "component_common.h"

void load_camera_drivers(component_t* component);
void set_camera_port_definition(component_t* camera);
void set_camera_settings(component_t* camera);

#endif
