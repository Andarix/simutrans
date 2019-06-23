/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef MAKEOBJ
#include "../descriptor/writer/obj_writer.h"
#define dr_fopen fopen
#endif

#include "../simsys.h"
#include "../simdebug.h"
#include "../descriptor/image.h"
#include "koord.h"
#include "tabfile.h"


bool tabfile_t::open(const char *filename)
{
	close();
	file = dr_fopen(filename, "r");
	return file != NULL;
}



void tabfile_t::close()
{
	if(file) {
		fclose(file);
		file  = NULL;
	}
}


const char *tabfileobj_t::get(const char *key)
{
	obj_info_t *result = objinfo.access(key);
	if(  result  ) {
		result->retrieved = true;
		return result->str;
	}
	return "";
}


/**
 * Get the string value for a key - key must be lowercase
 * @return def if key isn't found, value otherwise
 * @author Hj. Malthaner
 */
const char *tabfileobj_t::get_string(const char *key, const char * def)
{
	obj_info_t *result = objinfo.access(key);
	if(  result  ) {
		result->retrieved = true;
		return result->str;
	}
	return def;
}



bool tabfileobj_t::put(const char *key, const char *value)
{
	if(objinfo.get(key).str) {
		return false;
	}
	objinfo.put(strdup(key), obj_info_t(false,strdup(value)) );
	return true;
}



void tabfileobj_t::clear()
{
	FOR(stringhashtable_tpl<obj_info_t>, const& i, objinfo) {
		free(const_cast<char*>(i.key));
		free(const_cast<char*>(i.value.str));
	}
	objinfo.clear();
}


// private helps to get x y value pairs needed for koord etc.
template<class I>
bool tabfileobj_t::get_x_y( const char *key, I &x, I &y )
{
	const char *value = get_string(key,NULL);
	const char *tmp;

	if(!value) {
		return false;
	}
	// 2. Determine value
	for(tmp = value; *tmp != ','; tmp++) {
		if(!*tmp) {
			return false;
		}
	}
	x = atoi(value);
	y = atoi(tmp + 1);
	return true;
}

const koord &tabfileobj_t::get_koord(const char *key, koord def)
{
	static koord ret;
	ret = def;
	get_x_y( key, ret.x, ret.y );
	return ret;
}

const scr_size &tabfileobj_t::get_scr_size(const char *key, scr_size def)
{
	static scr_size ret;
	ret = def;
	get_x_y( key, ret.w, ret.h );
	return ret;
}

PIXVAL tabfileobj_t::get_color(const char *key, PIXVAL def, uint32 *color_rgb)
{
	const char *value = get_string(key,NULL);

	if(!value) {
		return def;
	}
	else {
		// skip spaces/tabs
		while ( *value>0  &&  *value<=32  ) {
			value ++;
		}
#ifndef MAKEOBJ
		if(  *value=='#'  ) {
			uint32 rgb = strtoul( value+1, NULL, 16 ) & 0XFFFFFFul;
			// we save in settings as RGB888
			if (color_rgb) {
				*color_rgb = rgb;
			}
			// but the functions expect in the system colour (like RGB565)
			return get_system_color( rgb>>16, (rgb>>8)&0xFF, rgb&0xFF );
		}
		else {
			// this inputs also hex correct
			uint8 index = (uint8)strtoul( value, NULL, 0 );
			// we save in settings as RGB888
			if (color_rgb) {
				*color_rgb = get_color_rgb(index);
			}
			// but the functions expect in the system colour (like RGB565)
			return color_idx_to_rgb(index);
		}
#else
		(void)color_rgb;
		return (uint8)strtoul( value, NULL, 0 );
#endif
	}
}

int tabfileobj_t::get_int(const char *key, int def)
{
	const char *value = get_string(key,NULL);

	if(!value) {
		return def;
	}
	else {
		// skip spaces/tabs
		while ( *value>0  &&  *value<=32  ) {
			value ++;
		}
		// this inputs also hex correct
		return strtol( value, NULL, 0 );
	}
}


sint64 atosint64(const char* a)
{
	return (sint64)(atof(a)+0.5);
}


sint64 tabfileobj_t::get_int64(const char *key, sint64 def)
{
	const char *value = get_string(key,NULL);

	if(!value) {
		return def;
	}
	else {
		return atosint64(value);
	}
}


int *tabfileobj_t::get_ints(const char *key)
{
	const char *value = get_string(key,NULL);
	const char *tmp;
	int         count = 1;
	int         *result;

	if(!value) {
		result = new int[1];
		result[0] = 0;
		return result;
	}
	// Determine number
	for(tmp = value; *tmp; tmp++) {
		if(*tmp == ',') {
			count++;
		}
	}
	// Create result vector and fill
	result = new int[count + 1];

	result[0] = count;
	count = 1;
	result[count++] = strtol( value, NULL, 0 );
	for(tmp = value; *tmp; tmp++) {
		if(*tmp == ',') {
			// skip spaces/tabs
			do {
				tmp ++;
			} while ( *tmp>0  &&  *tmp<=32  );
			// this inputs also hex correct
			result[count++] = strtol( tmp, NULL, 0 );
		}
	}
	return result;
}


sint64 *tabfileobj_t::get_sint64s(const char *key)
{
	const char *value = get_string(key,NULL);
	const char *tmp;
	int         count = 1;
	sint64         *result;

	if(!value) {
		result = new sint64[1];
		result[0] = 0;
		return result;
	}
	// Determine number
	for(tmp = value; *tmp; tmp++) {
		if(*tmp == ',') {
			count++;
		}
	}
	// Create result vector and fill
	result = new sint64[count + 1];

	result[0] = count;
	count = 1;
	result[count++] = atosint64(value);
	for(tmp = value; *tmp; tmp++) {
		if(*tmp == ',') {
			result[count++] = atosint64(tmp + 1);
		}
	}
	return result;
}


void tabfileobj_t::unused( const char *exclude_start_chars )
{
	FOR(stringhashtable_tpl<obj_info_t>, const& i, objinfo) {
		if(  !i.value.retrieved  ) {
			// never retrieved
			if(  !exclude_start_chars  ||  !strchr( exclude_start_chars, *i.value.str )  ) {
#ifdef MAKEOBJ
				dbg->warning( obj_writer_t::last_name, "Entry \"%s=%s\" ignored (check spelling)", i.key, i.value.str );
#else
				dbg->warning( "tabfile_t", "Entry \"%s=%s\" ignored (check spelling)", i.key, i.value.str );
#endif
			}
			objinfo.access( i.key )->retrieved = true;
		}
	}
}


bool tabfile_t::read(tabfileobj_t &objinfo, FILE *fp)
{
	bool lines = false;
	char line[4096];

	char line_expand[4096];
	char delim_expand[4096];
	char buffer[4096];
	char *param[10];
	char *expansion[10];

	objinfo.clear();

	do {
		while(read_line(line, sizeof(line)) && *line != '-') {
			char *delim = strchr(line, '=');

			if(delim) {
				*delim++ = '\0';
				format_key(line);

				int parameters = 0;
				int expansions = 0;
				/*
				@line, the whole parameter text (everything before =)
				@delim, the whole value text (everything after =)
				@parameters, number of fields enclosed by square brackets []
				@expansions, number of expansions included in the value (text inside angle brackets <>)
				@param, array containing the text inside each [] field
				@expansion, array containing the text inside each <> field
				 */
				if(find_parameter_expansion(line, delim, &parameters, &expansions, param, expansion) > 0) {
					int parameter_value[10][256];
					int parameter_length[10];
					int parameter_values[10]; // number of possible 'values' inside each [] field | e.g. [0-4]=5 / [n,s,w]=3
					char parameter_name[256][6]; // non-numeric ribis strings for all parameter fields consecutively
					bool parameter_ribi[10]; // true if parameters are ribi strings

					int combinations=1;
					int names = 0; // total number of ribi parameters

					// analyse and obtain all parameter expansions
					for(int i=0; i<parameters; i++) {
						char *token_ptr;
						parameter_length[i] = strcspn(param[i],"]");
						parameter_values[i] = 0;
						parameter_ribi[i] = false;

						sprintf(buffer, "%.*s", parameter_length[i], param[i]);
						int name_length = strcspn(buffer,",");

						int value = atoi(buffer);
						if (value == 0 && (tolower(buffer[0]) == 'n' || tolower(buffer[0]) == 's' || tolower(buffer[0]) == 'e' || tolower(buffer[0]) == 'w')) {
							sprintf(parameter_name[ names++ ], "%.*s", name_length, buffer);
							parameter_ribi[i] = true;
						}
						parameter_value[i][parameter_values[i]++] = value;

						token_ptr = strtok(buffer,"-,");
						while (token_ptr != NULL && parameter_values[i]<256) {
							switch(param[i][token_ptr-buffer-1]) {
								case ',':
									value = atoi(token_ptr);
									if (parameter_ribi[i]) {
										value = parameter_values[i];
										sprintf(parameter_name[ names++ ], "%.*s", (int)strcspn(buffer+name_length+1,","), buffer+name_length+1);
										name_length += strcspn(buffer+name_length+1,",") + 1;
									}
									parameter_value[i][parameter_values[i]++] = value;
								break;
								case '-':
									if (parameter_ribi[i]) {
										value = parameter_values[i];
										sprintf(parameter_name[value], "%.*s", (int)strcspn(buffer+name_length+1,","), buffer+name_length+1);
										name_length += strcspn(buffer+name_length+1,",") + 1;
										parameter_value[i][parameter_values[i]++] = value;
									}
									else {
										int start_range = parameter_value[i][parameter_values[i]-1];
										int end_range = atoi(token_ptr);
										for(int range=start_range; range<end_range; range++) {
											parameter_value[i][parameter_values[i]++] = range+1;
										}
									}
							}
							token_ptr = strtok(NULL, "-,");
						}
						combinations*=parameter_values[i];
					}

					// start expansion of the parameter
					for(int c=0; c<combinations; c++) {
						int names = 0;
						int combination[10];
						if(parameters>0) {
							// warp values around the number of parameters the expansion has
							for(int j=0; j<parameters; j++) {
								combination[j] = c;
								for(int k=0; k<j; k++) {
									combination[j]/=parameter_values[k];
								}
								combination[j]%=parameter_values[j];
							}
							char* prev_end = line;
							line_expand[0] = 0;
							for (int i=0; i<parameters; i++) {
								// if expanding non-numerical parameters
								if (parameter_ribi[i]) {
									sprintf(buffer, "%.*s%s", (int)(param[i]-prev_end), prev_end, parameter_name[ names + combination[i]]);
									names += parameter_values[i];
								}
								// if expanding numerical parameters
								else {
									sprintf(buffer, "%.*s%d", (int)(param[i]-prev_end), prev_end, parameter_value[i][combination[i]]);
								}
								strcat(line_expand, buffer);
								prev_end = param[i]+parameter_length[i];
							}
							strcat(line_expand, param[parameters-1]+parameter_length[parameters-1]);
						}
						else {
							strcpy(line_expand, line);
						}

						// expand the value
						if(expansions>0) {
							int expansion_length[10];
							int expansion_value[10];

							for(int i=0; i<expansions; i++) {
								expansion_length[i] = strcspn(expansion[i],">")-1;
								sprintf(buffer, "%.*s", expansion_length[i]+1, expansion[i]);

								expansion_value[i] = calculate(buffer, parameter_value, parameters, combination);
							}

							sprintf(delim_expand, "%.*s%d", (int)(expansion[0]-delim), delim, expansion_value[0]);
							for(int i=1; i<expansions; i++) {
								char *prev_end = expansion[i-1]+expansion_length[i-1]+2;
								sprintf(buffer, "%.*s%d", (int)(expansion[i]-prev_end), prev_end, expansion_value[i]);
								strcat(delim_expand, buffer);
							}
							strcat(delim_expand, expansion[expansions-1]+expansion_length[expansions-1]+2);
						}
						else {
							strcpy(delim_expand, delim);
						}

						printf("%s = %s\n", line_expand, delim_expand);
						objinfo.put(line_expand, delim_expand);
						if (fp != NULL) {
							fprintf(fp, "%s=%s\n", line_expand, delim_expand);
						}
					}
				}
				else {
					objinfo.put(line, delim);
					if (fp != NULL) {
						fprintf(fp, "%s=%s\n", line, delim);
					}
				}
				lines = true;
			}
			else if(  *line  ) {
				dbg->warning( "tabfile_t::read", "No data in \"%s\"", line );
			}
		}
	} while(!lines && !feof(file)); // skip empty objects

	return lines;
}



bool tabfile_t::read_line(char *s, int size)
{
	char *r;
	size_t l;

	do {
		r = fgets(s, size, file);
	} while(r != NULL  &&  (*s == '#' || *s == ' ')  );

	if(r) {
		l = strlen(r);
		while(  l  &&  (r[l-1] == '\n' || r[l-1] == '\r')  ) {
			r[--l] = '\0';
		}
	}
	return r != NULL;
}



int tabfile_t::find_parameter_expansion(char *key, char *data, int *parameters, int *expansions, char *param_ptr[10], char *expansion_ptr[10])
{
	char *s;

	// find params in key
	for(s = key; *s; s++) {
		if(*s == '[') {
			bool parameter = false;
			s++;

			char *t = s;

			while(*s && *s != ']') {
				if( (*s == ',' || *s == '-') && *(s-1) != '[' ) {
					parameter = true;
				}
				s++;
			}
			if(parameter) {
				param_ptr[*parameters] = t;
				(*parameters)++;
			}
		}
	}


	// find expansions in data
	for(s = data; *s; s++) {
		if(*s == '<') {
			char *t = s;
			while(*s) {
				if(*s == '>') {
					expansion_ptr[*expansions] = t;
					(*expansions)++;
					break;
				}
				s++;
			}
		}
	}

	return (*parameters)+(*expansions);
}



int tabfile_t::calculate(char *expression, int parameter_value[10][256], int parameters, int combination[10])
{
	char processed[4096];
	add_operator_brackets(expression, processed);
	return calculate_internal(processed, parameter_value, parameters, combination);
}



void tabfile_t::add_operator_brackets(char *expression, char *processed)
{
	char buffer[4096];
	char operators[5] = { '%','/','*','-','+' };

	// first remove any spaces
	int j = 0;
	for(uint16 i = 0; i<=strlen(expression); i++) {
		if(expression[i]!=' ') {
			buffer[j++]=expression[i];
		}
	}
	strcpy(processed, buffer);

	// add brackets around each operator to ensure processed in correct order
	// e.g. v+w*x+y/z becomes (((v+(w*x))+(y/z)) and x/y/z = ((x/y)/z)

	// for each operator take 'a operator b' and turn into '(a operator b)'

	for(int operator_loop = 0 ; operator_loop<5; operator_loop++) {
		int operator_count = 0;

		char *expression_ptr = strchr(processed, operators[operator_loop]);
		while(expression_ptr!=NULL) {
			// find a
			char *expression_pos = expression_ptr-1;
			char *expression_start = NULL;
			while(expression_pos>=processed && !expression_start) {
				switch(expression_pos[0]) {
					case ')': {
						int bracket_level = 1;
						expression_start = expression_pos-1;
						while(expression_start>processed && bracket_level>0) {
							switch(expression_start[0]) {
								case ')':
									bracket_level++;
								break;
								case '(':
									bracket_level--;
								break;
							}
							expression_start--;
						}
						if(bracket_level>0) dbg->fatal( "tabfile_t::read", "mismatched brackets" );
					}
					break;
					case '%':
					case '/':
					case '*':
					case '+':
					case '-':
					case '(':
					case '<':
						expression_start = expression_pos;
					break;
				}
				expression_pos--;
			}
			if(expression_start==NULL) expression_start=expression_pos;

			// find b
			expression_pos = expression_ptr+1;
			char *expression_end = NULL;
			while(expression_pos[0]!='\0' && !expression_end) {
				switch(expression_pos[0]) {
					case '(': {
						int bracket_level = 1;
						expression_end = expression_pos+1;
						while(expression_end[0]!='\0' && bracket_level>0) {
							switch(expression_end[0]) {
								case '(':
									bracket_level++;
								break;
								case ')':
									bracket_level--;
								break;
							}
							expression_end++;
						}
						if(bracket_level>0) dbg->fatal( "tabfile_t::read", "mismatched brackets" );
					}
					break;
					case '%':
					case '/':
					case '*':
					case '+':
					case '-':
						expression_end = expression_pos;
					break;
				}
				expression_pos++;
			}
			if(expression_end==NULL) expression_end = expression_pos;

			// construct expression with brackets around 'a operator b'
			sprintf(buffer,"%.*s(%.*s%.*s)%s",	(int)(expression_start-processed+1), processed,
								(int)(expression_ptr-expression_start-1), expression_start+1,
								(int)(expression_end-expression_ptr), expression_ptr,
								expression_end);

			strcpy(processed, buffer);
			operator_count++;

			expression_ptr = strchr(processed, operators[operator_loop]);
			for(int i=0; i<operator_count; i++) {
				expression_ptr = strchr(expression_ptr+1, operators[operator_loop]);
			}
		}
	}
}



int tabfile_t::calculate_internal(char *expression, int parameter_value[10][256], int parameters, int combination[10])
{
	int answer = 0;

	char original[4096];
	strcpy(original, expression);

	char *token_ptr = strtok(expression,"<-+*/%");

	if(token_ptr[0]=='(') {
		token_ptr++;
	}

	while (token_ptr<expression+strlen(original)) {
		int value = 0;
		char operator_char = original[token_ptr-expression-1];

		if(token_ptr[0]=='$') {
			int parameter = atoi((char *)(token_ptr+1));
			if(parameter<=parameters) {
				value = parameter_value[parameter][combination[parameter]];
			}
		}
		else {
			if(token_ptr[0]=='(') {
				uint16 bracket_length;
				int bracket_level=1;
				for(bracket_length=1; bracket_length<strlen(original)-(token_ptr-expression) && bracket_level>0; bracket_length++) {
					switch(original[token_ptr-expression+bracket_length]) {
						case '(':
							bracket_level++;
						break;
						case ')':
							bracket_level--;
						break;
					}
				}
				char buffer[4096];
				sprintf(buffer, "%.*s", bracket_length, original+(token_ptr-expression)-1);
				value = calculate_internal(buffer+1, parameter_value, parameters, combination);
				token_ptr+=bracket_length;
			}
			else {
				value = atoi(token_ptr);
			}
		}
		switch(operator_char) {
			case '+':
				answer+= value;
			break;
			case '-':
				answer-= value;
			break;
			case '*':
				answer*= value;
			break;
			case '/':
				answer/= value;
			break;
			case '%':
				answer%= value;
			break;
			case '<':
			case '(':
				answer = value;
			break;
		}
		token_ptr += strcspn(token_ptr, "<-+*/%")+1;
	}
	return answer;
}



void tabfile_t::format_key(char *key)
{
	char *s = key + strlen(key);
	char *t;

	// trim right
	while(s > key && s[-1] == ' ') {
		*--s = '\0';
	}
	// make lowercase
	for(s = key; *s; s++) {
		*s = tolower(*s);
	}
	// skip spaces inside []
	for(s = t = key; *s; s++) {
		if(*s == '[') {
			*t++ = *s++;

			while(*s && *s != ']') {
				if(*s == ' ') {
					s++;
				}
				else {
					*t++ = *s++;
				}
			}
			s--;
		}
		else {
			*t++ = *s;
		}
	}
	*t = '\0';
}
