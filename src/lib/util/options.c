/***************************************************************************

    options.c

    Core options code code

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include <stdarg.h>
#include <ctype.h>
#include "options.h"
#include "pool.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_ENTRY_NAMES		4



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _options_data options_data;
struct _options_data
{
	options_data *			next;				/* link to the next data */
	const char *			names[MAX_ENTRY_NAMES]; /* array of possible names */
	UINT32					flags;				/* flags from the entry */
	UINT32					seqid;				/* sequence ID; bumped on each change */
	int						error_reported;		/* have we reported an error on this option yet? */
	const char *			data;				/* data for this item */
	const char *			defdata;			/* default data for this item */
	const char *			description;		/* description for this item */
	void					(*callback)(core_options *opts, const char *arg);	/* callback to be invoked when parsing */
};



struct _core_options
{
	memory_pool *pool;
	void (*output[OPTMSG_COUNT])(const char *s);

	/* linked list containing option data */
	options_data *datalist;
	options_data **datalist_nextptr;
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static options_data *find_entry_data(core_options *opts, const char *string, int is_command_line);
static void update_data(core_options *opts, options_data *data, const char *newdata);



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

const char *option_unadorned[MAX_UNADORNED_OPTIONS] =
{
	"<UNADORNED0>",
	"<UNADORNED1>",
	"<UNADORNED2>",
	"<UNADORNED3>",
	"<UNADORNED4>",
	"<UNADORNED5>",
	"<UNADORNED6>",
	"<UNADORNED7>",
	"<UNADORNED8>",
	"<UNADORNED9>",
	"<UNADORNED10>",
	"<UNADORNED11>",
	"<UNADORNED12>",
	"<UNADORNED13>",
	"<UNADORNED14>",
	"<UNADORNED15>"
};



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    message - outputs a message to a listener
-------------------------------------------------*/

static void message(core_options *opts, options_message msgtype, const char *format, ...)
{
	char buf[1024];
	va_list argptr;

	if (opts->output[msgtype])
	{
		va_start(argptr, format);
		vsprintf(buf, format, argptr);
		va_end(argptr);

		opts->output[msgtype](buf);
	}
}



/*-------------------------------------------------
    output_printf - outputs an arbitrary message
    to a callback
-------------------------------------------------*/

static void output_printf(void (*output)(const char *s), const char *format, ...)
{
	char buf[1024];
	va_list argptr;

	va_start(argptr, format);
	vsprintf(buf, format, argptr);
	va_end(argptr);

	output(buf);
}



/*-------------------------------------------------
    copy_string - allocate a copy of a string
-------------------------------------------------*/

static const char *copy_string(core_options *opts, const char *start, const char *end)
{
	char *result;

	/* copy of a NULL is NULL */
	if (start == NULL)
		return NULL;

	/* if no end, figure it out */
	if (end == NULL)
		end = start + strlen(start);

	/* allocate, copy, and NULL-terminate */
	result = pool_malloc(opts->pool, (end - start) + 1);
	if (result == NULL)
		return NULL;
	memcpy(result, start, end - start);
	result[end - start] = 0;

	return result;
}


/*-------------------------------------------------
    separate_names - separate a list of semicolon
    separated names into individual strings
-------------------------------------------------*/

static int separate_names(core_options *opts, const char *srcstring, const char *results[], int maxentries)
{
	const char *start;
	int curentry;

	/* start with the original string and loop over entries */
	start = srcstring;
	for (curentry = 0; curentry < maxentries; curentry++)
	{
		/* find the end of this entry and copy the string */
		const char *end = strchr(start, ';');
		results[curentry] = copy_string(opts, start, end);

		/* if we hit the end of the source, stop */
		if (end == NULL)
			break;
		start = end + 1;
	}
	return curentry;
}


/*-------------------------------------------------
    options_create - creates a new instance of
    core options
-------------------------------------------------*/

core_options *options_create(void (*fail)(const char *message))
{
	memory_pool *pool;
	core_options *opts;

	/* create a pool for this option block */
	pool = pool_create(fail);
	if (!pool)
		goto error;

	/* allocate memory for the option block */
	opts = (core_options *) pool_malloc(pool, sizeof(*opts));
	if (!opts)
		goto error;

	/* and set up the structure */
	memset(opts, '\0', sizeof(*opts));
	opts->pool = pool;
	opts->datalist_nextptr = &opts->datalist;
	return opts;

error:
	if (pool)
		pool_free(pool);
	return NULL;
}



/*-------------------------------------------------
    options_free - frees an options object
-------------------------------------------------*/

void options_free(core_options *opts)
{
	pool_free(opts->pool);
}



/*-------------------------------------------------
    options_set_output_callback - installs a callback
    to be invoked when data is outputted in the
    process of outputting options
-------------------------------------------------*/

void options_set_output_callback(core_options *opts, options_message msgtype, void (*callback)(const char *s))
{
	opts->output[msgtype] = callback;
}



/*-------------------------------------------------
    options_add_entries - add entries to the
    current options sets
-------------------------------------------------*/

int options_add_entries(core_options *opts, const options_entry *entrylist)
{
	/* loop over entries until we hit a NULL name */
	for ( ; entrylist->name != NULL || (entrylist->flags & OPTION_HEADER); entrylist++)
	{
		options_data *match = NULL;
		int i;

		/* allocate a new item */
		options_data *data = pool_malloc(opts->pool, sizeof(*data));
		if (!data)
			return FALSE;
		memset(data, 0, sizeof(*data));

		/* separate the names */
		if (entrylist->name != NULL)
			separate_names(opts, entrylist->name, data->names, ARRAY_LENGTH(data->names));

		/* do we match an existing entry? */
		for (i = 0; i < ARRAY_LENGTH(data->names) && match == NULL; i++)
			if (data->names[i] != NULL)
				match = find_entry_data(opts, data->names[i], FALSE);

		/* if so, throw away this entry and replace the data */
		if (match != NULL)
		{
			/* free what we've allocated so far */
			for (i = 0; i < ARRAY_LENGTH(data->names); i++)
				if (data->names[i] != NULL)
					pool_realloc(opts->pool, (void *)data->names[i], 0);
			pool_realloc(opts->pool, data, 0);

			/* update the data and default data */
			data = match;
			if (data->data != NULL)
				pool_realloc(opts->pool, (void *)data->data, 0);
			if (data->defdata != NULL)
				pool_realloc(opts->pool, (void *)data->defdata, 0);
			data->data = copy_string(opts, entrylist->defvalue, NULL);
			data->defdata = copy_string(opts, entrylist->defvalue, NULL);
		}

		/* otherwise, finish making the new entry */
		else
		{
			/* copy the flags, and set the value equal to the default */
			data->flags = entrylist->flags;
			data->data = copy_string(opts, entrylist->defvalue, NULL);
			data->defdata = copy_string(opts, entrylist->defvalue, NULL);
			data->description = entrylist->description;

			/* fill it in and add to the end of the list */
			*(opts->datalist_nextptr) = data;
			opts->datalist_nextptr = &data->next;
		}
	}
	return TRUE;
}


/*-------------------------------------------------
    options_set_option_default_value - change the
    default value of an option
-------------------------------------------------*/

int options_set_option_default_value(core_options *opts, const char *name, const char *defvalue)
{
	options_data *data = find_entry_data(opts, name, TRUE);

	/* free the existing value and make a copy for the new one */
	/* note that we assume this is called before any data is processed */
	if (data->data != NULL)
		pool_realloc(opts->pool, (void *)data->data, 0);
	if (data->defdata != NULL)
		pool_realloc(opts->pool, (void *)data->defdata, 0);
	data->data = copy_string(opts, defvalue, NULL);
	data->defdata = copy_string(opts, defvalue, NULL);
	return TRUE;
}


/*-------------------------------------------------
    options_set_option_callback - specifies a
    callback to be invoked when parsing options
-------------------------------------------------*/

int options_set_option_callback(core_options *opts, const char *name, void (*callback)(core_options *opts, const char *arg))
{
	options_data *data = find_entry_data(opts, name, TRUE);
	if (!data)
		return FALSE;
	data->callback = callback;
	return TRUE;
}


/*-------------------------------------------------
    options_parse_command_line - parse a series
    of command line arguments
-------------------------------------------------*/

int options_parse_command_line(core_options *opts, int argc, char **argv)
{
	int unadorned_index = 0;
	int arg;

	/* loop over commands, looking for options */
	for (arg = 1; arg < argc; arg++)
	{
		options_data *data;
		const char *optionname;
		const char *newdata;
		int is_unadorned;

		/* determine the entry name to search for */
		is_unadorned = (argv[arg][0] != '-');
		if (!is_unadorned)
			optionname = &argv[arg][1];
		else
			optionname = OPTION_UNADORNED(unadorned_index);

		/* find our entry */
		data = find_entry_data(opts, optionname, TRUE);
		if (data == NULL)
		{
			message(opts, OPTMSG_ERROR, "Error: unknown option: %s\n", argv[arg]);
			return 1;
		}
		if ((data->flags & (OPTION_DEPRECATED | OPTION_INTERNAL)) != 0)
			continue;

		/* if unadorned, we have to bump the count (unless if the option repeats) */
		if (is_unadorned && !(data->flags & OPTION_REPEATS))
			unadorned_index++;

		/* get the data for this argument, special casing booleans */
		if ((data->flags & (OPTION_BOOLEAN | OPTION_COMMAND)) != 0)
			newdata = (strncmp(&argv[arg][1], "no", 2) == 0) ? "0" : "1";
		else if (argv[arg][0] != '-')
			newdata = argv[arg];
		else if (arg + 1 < argc)
			newdata = argv[++arg];
		else
		{
			message(opts, OPTMSG_ERROR, "Error: option %s expected a parameter\n", argv[arg]);
			return 1;
		}

		/* invoke callback, if present */
		if (data->callback != NULL)
			(*data->callback)(opts, newdata);

		/* allocate a new copy of data for this */
		update_data(opts, data, newdata);
	}
	return 0;
}


/*-------------------------------------------------
    options_parse_ini_file - parse a series
    of entries in an INI file
-------------------------------------------------*/

int options_parse_ini_file(core_options *opts, core_file *inifile)
{
	char buffer[1024];

	/* loop over data */
	while (core_fgets(buffer, ARRAY_LENGTH(buffer), inifile) != NULL)
	{
		char *optionname, *optiondata, *temp;
		options_data *data;
		int inquotes = FALSE;

		/* find the name */
		for (optionname = buffer; *optionname != 0; optionname++)
			if (!isspace(*optionname))
				break;

		/* skip comments */
		if (*optionname == 0 || *optionname == '#')
			continue;

		/* scan forward to find the first space */
		for (temp = optionname; *temp != 0; temp++)
			if (isspace(*temp))
				break;

		/* if we hit the end early, print a warning and continue */
		if (*temp == 0)
		{
			message(opts, OPTMSG_WARNING, "Warning: invalid line in INI: %s", buffer);
			continue;
		}

		/* NULL-terminate */
		*temp++ = 0;
		optiondata = temp;

		/* scan the data, stopping when we hit a comment */
		for (temp = optiondata; *temp != 0; temp++)
		{
			if (*temp == '"')
				inquotes = !inquotes;
			if (*temp == '#' && !inquotes)
				break;
		}
		*temp = 0;

		/* find our entry */
		data = find_entry_data(opts, optionname, FALSE);
		if (data == NULL)
		{
			message(opts, OPTMSG_WARNING, "Warning: unknown option in INI: %s\n", optionname);
			continue;
		}
		if ((data->flags & (OPTION_DEPRECATED | OPTION_INTERNAL)) != 0)
			continue;

		/* allocate a new copy of data for this */
		update_data(opts, data, optiondata);
	}
	return 0;
}


/*-------------------------------------------------
    options_output_ini_file - output the current
    state to an INI file
-------------------------------------------------*/

void options_output_ini_file(core_options *opts, core_file *inifile)
{
	options_data *data;

	/* loop over all items */
	for (data = opts->datalist; data != NULL; data = data->next)
	{
		/* header: just print */
		if ((data->flags & OPTION_HEADER) != 0)
			core_fprintf(inifile, "\n#\n# %s\n#\n", data->description);

		/* otherwise, output entries for all non-deprecated and non-command items */
		else if ((data->flags & (OPTION_DEPRECATED | OPTION_INTERNAL | OPTION_COMMAND)) == 0 && data->names[0][0] != 0)
		{
			if (data->data == NULL)
				core_fprintf(inifile, "# %-23s <NULL> (not set)\n", data->names[0]);
			else if (strchr(data->data, ' ') != NULL)
				core_fprintf(inifile, "%-25s \"%s\"\n", data->names[0], data->data);
			else
				core_fprintf(inifile, "%-25s %s\n", data->names[0], data->data);
		}
	}
}


/*-------------------------------------------------
    options_output_ini_file - output the current
    state to an INI file
-------------------------------------------------*/

void options_output_ini_stdfile(core_options *opts, FILE *inifile)
{
	options_data *data;

	/* loop over all items */
	for (data = opts->datalist; data != NULL; data = data->next)
	{
		/* header: just print */
		if ((data->flags & OPTION_HEADER) != 0)
			fprintf(inifile, "\n#\n# %s\n#\n", data->description);

		/* otherwise, output entries for all non-deprecated and non-command items */
		else if ((data->flags & (OPTION_DEPRECATED | OPTION_INTERNAL | OPTION_COMMAND)) == 0 && data->names[0][0] != 0)
		{
			if (data->data == NULL)
				fprintf(inifile, "# %-23s <NULL> (not set)\n", data->names[0]);
			else if (strchr(data->data, ' ') != NULL)
				fprintf(inifile, "%-25s \"%s\"\n", data->names[0], data->data);
			else
				fprintf(inifile, "%-25s %s\n", data->names[0], data->data);
		}
	}
}


/*-------------------------------------------------
    options_output_help - output option help to
    a file
-------------------------------------------------*/

void options_output_help(core_options *opts, void (*output)(const char *))
{
	options_data *data;

	/* loop over all items */
	for (data = opts->datalist; data != NULL; data = data->next)
	{
		/* header: just print */
		if ((data->flags & OPTION_HEADER) != 0)
			output_printf(output, "\n#\n# %s\n#\n", data->description);

		/* otherwise, output entries for all non-deprecated items */
		else if ((data->flags & (OPTION_DEPRECATED | OPTION_INTERNAL)) == 0 && data->description != NULL)
			output_printf(output, "-%-20s%s\n", data->names[0], data->description);
	}
}


/*-------------------------------------------------
    options_copy - copy options from one core_options
    to another
-------------------------------------------------*/

int options_copy(core_options *dest_opts, core_options *src_opts)
{
	options_data *data;
	const char *name;
	const char *value;

	for (data = dest_opts->datalist; data != NULL; data = data->next)
	{
		if (!(data->flags & OPTION_HEADER))
		{
			name = data->names[0];
			value = options_get_string(src_opts, name);
			if (value != NULL)
				options_set_string(dest_opts, name, value);
		}
	}
	return TRUE;
}


/*-------------------------------------------------
    options_equal - compare two sets of options
-------------------------------------------------*/

int options_equal(core_options *opts1, core_options *opts2)
{
	options_data *data;
	const char *value1;
	const char *value2;

	for (data = opts1->datalist; data != NULL; data = data->next)
	{
		if (!(data->flags & OPTION_HEADER))
		{
			value1 = options_get_string(opts1, data->names[0]);
			value2 = options_get_string(opts2, data->names[0]);
			value1 = value1 ? value1 : "";
			value2 = value2 ? value2 : "";
			if (strcmp(value1, value2))
				return FALSE;
		}
	}
	return TRUE;
}



/*-------------------------------------------------
    options_get_string - return data formatted
    as a string
-------------------------------------------------*/

const char *options_get_string(core_options *opts, const char *name)
{
	options_data *data = find_entry_data(opts, name, FALSE);
	return (data == NULL) ? "" : data->data;
}


/*-------------------------------------------------
    options_get_seqid - return the seqid for an
    entry
-------------------------------------------------*/

UINT32 options_get_seqid(core_options *opts, const char *name)
{
	options_data *data = find_entry_data(opts, name, FALSE);
	return (data == NULL) ? 0 : data->seqid;
}


/*-------------------------------------------------
    options_get_bool - return data formatted as
    a boolean
-------------------------------------------------*/

int options_get_bool(core_options *opts, const char *name)
{
	options_data *data = find_entry_data(opts, name, FALSE);
	int value = FALSE;

	if (data == NULL)
		message(opts, OPTMSG_ERROR, "Unexpected boolean option %s queried\n", name);
	else if (data->data == NULL || sscanf(data->data, "%d", &value) != 1 || value < 0 || value > 1)
	{
		if (data->defdata != NULL)
		{
			options_set_string(opts, name, data->defdata);
			sscanf(data->data, "%d", &value);
		}
		if (!data->error_reported)
		{
			message(opts, OPTMSG_ERROR, "Illegal boolean value for %s; reverting to %d\n", data->names[0], value);
			data->error_reported = TRUE;
		}
	}
	return value;
}


/*-------------------------------------------------
    options_get_int - return data formatted as
    an integer
-------------------------------------------------*/

int options_get_int(core_options *opts, const char *name)
{
	options_data *data = find_entry_data(opts, name, FALSE);
	int value = 0;

	if (data == NULL)
		message(opts, OPTMSG_ERROR, "Unexpected integer option %s queried\n", name);
	else if (data->data == NULL || sscanf(data->data, "%d", &value) != 1)
	{
		if (data->defdata != NULL)
		{
			options_set_string(opts, name, data->defdata);
			sscanf(data->data, "%d", &value);
		}
		if (!data->error_reported)
		{
			message(opts, OPTMSG_ERROR, "Illegal integer value for %s; reverting to %d\n", data->names[0], value);
			data->error_reported = TRUE;
		}
	}
	return value;
}


/*-------------------------------------------------
    options_get_float - return data formatted as
    a float
-------------------------------------------------*/

float options_get_float(core_options *opts, const char *name)
{
	options_data *data = find_entry_data(opts, name, FALSE);
	float value = 0;

	if (data == NULL)
		message(opts, OPTMSG_ERROR, "Unexpected float option %s queried\n", name);
	else if (data->data == NULL || sscanf(data->data, "%f", &value) != 1)
	{
		if (data->defdata != NULL)
		{
			options_set_string(opts, name, data->defdata);
			sscanf(data->data, "%f", &value);
		}
		if (!data->error_reported)
		{
			message(opts, OPTMSG_ERROR, "Illegal float value for %s; reverting to %f\n", data->names[0], (double)value);
			data->error_reported = TRUE;
		}
	}
	return value;
}


/*-------------------------------------------------
    options_get_int_range - return data formatted
    as an integer and clamped to within the given
    range
-------------------------------------------------*/

int options_get_int_range(core_options *opts, const char *name, int minval, int maxval)
{
	options_data *data = find_entry_data(opts, name, FALSE);
	int value = 0;

	if (data == NULL)
		message(opts, OPTMSG_ERROR, "Unexpected integer option %s queried\n", name);
	else if (data->data == NULL || sscanf(data->data, "%d", &value) != 1)
	{
		if (data->defdata != NULL)
		{
			options_set_string(opts, name, data->defdata);
			value = options_get_int(opts, name);
		}
		if (!data->error_reported)
		{
			message(opts, OPTMSG_ERROR, "Illegal integer value for %s; reverting to %d\n", data->names[0], value);
			data->error_reported = TRUE;
		}
	}
	else if (value < minval || value > maxval)
	{
		options_set_string(opts, name, data->defdata);
		value = options_get_int(opts, name);
		if (!data->error_reported)
		{
			message(opts, OPTMSG_ERROR, "Invalid %s value (must be between %d and %d); reverting to %d\n", data->names[0], minval, maxval, value);
			data->error_reported = TRUE;
		}
	}

	return value;
}


/*-------------------------------------------------
    options_get_float_range - return data formatted
    as a float and clamped to within the given
    range
-------------------------------------------------*/

float options_get_float_range(core_options *opts, const char *name, float minval, float maxval)
{
	options_data *data = find_entry_data(opts, name, FALSE);
	float value = 0;

	if (data == NULL)
		message(opts, OPTMSG_ERROR, "Unexpected float option %s queried\n", name);
	else if (data->data == NULL || sscanf(data->data, "%f", &value) != 1)
	{
		if (data->defdata != NULL)
		{
			options_set_string(opts, name, data->defdata);
			value = options_get_float(opts, name);
		}
		if (!data->error_reported)
		{
			message(opts, OPTMSG_ERROR, "Illegal float value for %s; reverting to %f\n", data->names[0], (double)value);
			data->error_reported = TRUE;
		}
	}
	else if (value < minval || value > maxval)
	{
		options_set_string(opts, name, data->defdata);
		value = options_get_float(opts, name);
		if (!data->error_reported)
		{
			message(opts, OPTMSG_ERROR, "Invalid %s value (must be between %f and %f); reverting to %f\n", data->names[0], (double)minval, (double)maxval, (double)value);
			data->error_reported = TRUE;
		}
	}
	return value;
}


/*-------------------------------------------------
    options_set_string - set a string value
-------------------------------------------------*/

void options_set_string(core_options *opts, const char *name, const char *value)
{
	options_data *data = find_entry_data(opts, name, FALSE);
	update_data(opts, data, value);
}


/*-------------------------------------------------
    options_set_bool - set a boolean value
-------------------------------------------------*/

void options_set_bool(core_options *opts, const char *name, int value)
{
	char temp[4];
	sprintf(temp, "%d", value ? 1 : 0);
	options_set_string(opts, name, temp);
}


/*-------------------------------------------------
    options_set_int - set an integer value
-------------------------------------------------*/

void options_set_int(core_options *opts, const char *name, int value)
{
	char temp[20];
	sprintf(temp, "%d", value);
	options_set_string(opts, name, temp);
}


/*-------------------------------------------------
    options_set_float - set a float value
-------------------------------------------------*/

void options_set_float(core_options *opts, const char *name, float value)
{
	char temp[100];
	sprintf(temp, "%f", value);
	options_set_string(opts, name, temp);
}


/*-------------------------------------------------
    find_entry_data - locate an entry whose name
    matches the given string
-------------------------------------------------*/

static options_data *find_entry_data(core_options *opts, const char *string, int is_command_line)
{
	options_data *data;
	int has_no_prefix;

	/* determine up front if we should look for "no" boolean options */
	has_no_prefix = (is_command_line && string[0] == 'n' && string[1] == 'o');

	/* scan all entries */
	for (data = opts->datalist; data != NULL; data = data->next)
		if (!(data->flags & OPTION_HEADER))
		{
			const char *compareme = (has_no_prefix && (data->flags & OPTION_BOOLEAN)) ? &string[2] : &string[0];
			int namenum;

			/* loop over names */
			for (namenum = 0; namenum < ARRAY_LENGTH(data->names); namenum++)
				if (data->names[namenum] != NULL && strcmp(compareme, data->names[namenum]) == 0)
					return data;
		}

	/* didn't find it at all */
	return NULL;
}


/*-------------------------------------------------
    update_data - update the data value for a
    given entry
-------------------------------------------------*/

static void update_data(core_options *opts, options_data *data, const char *newdata)
{
	const char *dataend = newdata + strlen(newdata) - 1;
	const char *datastart = newdata;

	/* strip off leading/trailing spaces */
	while (isspace(*datastart) && datastart <= dataend)
		datastart++;
	while (isspace(*dataend) && datastart <= dataend)
		dataend--;

	/* strip off quotes */
	if (datastart != dataend && *datastart == '"' && *dataend == '"')
		datastart++, dataend--;

	/* allocate a copy of the data */
	if (data->data)
		pool_realloc(opts->pool, (void *)data->data, 0);
	data->data = copy_string(opts, datastart, dataend + 1);

	/* bump the seqid and clear the error reporting */
	data->seqid++;
	data->error_reported = FALSE;
}


/*-------------------------------------------------
    options_get_first_entry - begins enumerating
    through options entries
-------------------------------------------------*/

const options_entry *options_get_first_entry(core_options *opts)
{
	return NULL;
}


/*-------------------------------------------------
    options_get_next_entry - begins enumerating
    through options entries
-------------------------------------------------*/

const options_entry *options_get_next_entry(core_options *opts, const options_entry *entry)
{
	return NULL;
}