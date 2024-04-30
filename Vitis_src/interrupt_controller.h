#ifndef INTERRUPT_CONTROLLER_H /* prevent circular inclusions */
#define INTERRUPT_CONTROLLER_H /* by using protection macros */

#include "xscugic.h"

int init_interrupt_controller();

#endif // INTERRUPT_CONTROLLER_H
