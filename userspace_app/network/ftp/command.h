/*
 * =====================================================================================
 *
 *       Filename:  command.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/23/2014 11:31:09 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Wei Yang (weiyang), weiyang.kernel@gmail.com
 *        Company:  
 *
 * =====================================================================================
 */
#ifndef _COMMAND_H_
#define _COMMAND_H_

#include <stdio.h>

typedef int (*command_t)(char *);

struct command {
	char *name;
	command_t handle;
};

int handle_command(char *raw);

#endif //_COMMAND_H_
