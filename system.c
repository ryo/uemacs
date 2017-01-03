/*
 * system.c for XC
 *	Public domain: written by Kaoru Maeda, August 1990.
 *  little modified by SALT
 */

#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <string.h>
#include <errno.h>
#include "estruct.h"
#include "edef.h"
#include "etype.h"

int system(const char *arg)
{
	char shell[NSTRING], shell_opt[NSTRING], shell_quote[NSTRING];
	char sh_name[256], sh_arg[256];
	char *env;

	env = dosgetenv("ESHELL");
	if (env == NULL) {
		env = dosgetenv("SHELL");
		if (env == NULL)
			env = "command.x";
	}
	strcpy (shell, env);

	env = dosgetenv("ESHELL_OPT");
	if (env == NULL)
		env = "";
	strcpy (shell_opt, env);

	env = dosgetenv("ESHELL_QUOTE");
	if (env == NULL)
		env = "";
	strcpy (shell_quote, env);

	if (strlen(arg) > 254) {
		errno = E2BIG;
		return E2BIG << 8;
	}
	{
		char c;
		char *p, *q;

		strcpy(sh_arg, arg);
		for(p = q = sh_arg; c = *p; p++) {
			if (c != ' ' && c != '\t')
				break;
		}
		while (*q++ = *p++);
	}

	{
		int len;

		len = strlen(shell);
		if (*arg) {
			if (*shell_opt)
				len += strlen(shell_opt) + 3;
			len += strlen(arg) + 1;
		}
		if (len > 254) {
			errno = E2BIG;
			return E2BIG << 8;
		}
	}

	{
		char *p;
		char c;

		strcpy(sh_name, shell);
		if (*arg) {
			if (*shell_opt) {
				strcat(sh_name, " ");
				strcat(sh_name, shell_opt);
			}
			strcat(sh_name, " ");
			if (*shell_opt)
				strcat(sh_name, shell_quote);
			strcat(sh_name, arg);
			if (*shell_opt)
				strcat(sh_name, shell_quote);
		}

		for(p = sh_name; c = *p; p++) {
			if (c == '/')
				*p = '\\';
			else if (c == ' ' || c == '\t') {
				*p++ = 0;
				while ((c = *p++) && (c == ' ' || c == '\t'));
				p--;
				break;
			}
		}
		strcpy(sh_arg, p);
	}

	{
		int exe;

		if (debug_system) {
			char buffer[256];

			sprintf(buffer, "spawnlp (P_WAIT, \"%s\", \"%s\", \"%s\", NULL);", sh_name, sh_name, sh_arg);
			mlforce(buffer);
			getkey();
		}
		exe = spawnlp(P_WAIT, sh_name, sh_name, sh_arg, NULL);
		if (exe < 0)
			return errno << 8;

		return exe;
	}
}
