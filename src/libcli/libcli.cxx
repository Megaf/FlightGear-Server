#include <exception>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <regex.h>
#include "libcli.hxx"

namespace LIBCLI
{

using namespace std;


#ifndef CALL_MEMBER_FN
	CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))
#endif

class match_filter_state
{
public:
	int flags;
	union
	{
		char* str;
		regex_t re;
	} match;
};

class range_filter_state
{
public:
	int matched;
	char* from;
	char* to;
};

filter_cmds_t filter_cmds[] =
{
	{ "begin",   "Begin with lines that match" },
	{ "between", "Between lines that match" },
	{ "count",   "Count of lines"   },
	{ "exclude", "Exclude lines that match" },
	{ "include", "Include lines that match" },
	{ "grep",    "Include lines that match regex (options: -v, -i, -e)" },
	{ "egrep",   "Include lines that match extended regex" },
	{ NULL, NULL}
};

void
CLI::set_auth_callback
(
        c_auth_func callback
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	this->auth_callback = callback;
	this->cpp_auth_callback = 0;
}

void
CLI::set_auth_callback
(
        cpp_auth_func callback
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	this->auth_callback = 0;
	this->cpp_auth_callback = callback;
}

void
CLI::set_enable_callback
(
        c_enable_func callback
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	this->enable_callback = callback;
	this->cpp_enable_callback = 0;
}

void
CLI::set_enable_callback
(
        cpp_enable_func callback
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	this->enable_callback = 0;
	this->cpp_enable_callback = callback;
}

void
CLI::allow_user
(
        char* username,
        char* password
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	unp* u, *n;
	if ( ! ( n = ( unp* ) malloc ( sizeof ( unp ) ) ) )
	{
		cerr << "Couldn't allocate memory for user: " << strerror ( errno ) << endl;
		return;
	}
	if ( ! ( n->username = strdup ( username ) ) )
	{
		fprintf ( stderr, "Couldn't allocate memory for username: %s", strerror ( errno ) );
		free ( n );
		return;
	}
	if ( ! ( n->password = strdup ( password ) ) )
	{
		fprintf ( stderr, "Couldn't allocate memory for password: %s", strerror ( errno ) );
		free ( n->username );
		free ( n );
		return;
	}
	n->next = NULL;
	if ( ! this->users )
	{
		this->users = n;
	}
	else
	{
		for ( u = this->users; u && u->next; u = u->next )
			/* intentionally empty */
		{
			;
		}
		if ( u )
		{
			u->next = n;
		}
	}
}

void
CLI::allow_enable
(
        char* password
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	free_z ( this->enable_password );
	if ( ! ( this->enable_password = strdup ( password ) ) )
	{
		fprintf ( stderr, "Couldn't allocate memory for enable password: %s", strerror ( errno ) );
	}
}

void
CLI::deny_user
(
        char* username
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	unp* u, *p = NULL;
	if ( ! this->users )
	{
		return;
	}
	for ( u = this->users; u; u = u->next )
	{
		if ( strcmp ( username, u->username ) == 0 )
		{
			if ( p )
			{
				p->next = u->next;
			}
			else
			{
				this->users = u->next;
			}
			free ( u->username );
			free ( u->password );
			free ( u );
			break;
		}
		p = u;
	}
}

void
CLI::set_banner
(
        const char* banner
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	free_z ( this->banner );
	if ( banner && *banner )
	{
		this->banner = strdup ( banner );
	}
}

void
CLI::set_hostname
(
        const char* hostname
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	free_z ( this->hostname );
	if ( hostname && *hostname )
	{
		this->hostname = strdup ( hostname );
	}
}

void
CLI::set_promptchar
(
        const char* prompt
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	free_z ( this->promptchar );
	this->promptchar = strdup ( prompt );
}

int
CLI::build_shortest
(
        Command<CLI> *commands
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	Command<CLI> *c, *p;
	char* cp, *pp;
	unsigned int len;
	for ( c = commands; c; c = c->next )
	{
		c->unique_len = strlen ( c->command );
		if ( ( c->mode != MODE_ANY && c->mode != this->mode )
		                ||  ( c->privilege > this->privilege ) )
		{
			continue;
		}
		c->unique_len = 1;
		for ( p = commands; p; p = p->next )
		{
			if ( c == p )
			{
				continue;
			}
			if ( ( p->mode != MODE_ANY && p->mode != this->mode )
			                ||  ( p->privilege > this->privilege ) )
			{
				continue;
			}
			cp = c->command;
			pp = p->command;
			len = 0;
			while ( *cp && *pp && *cp++ == *pp++ )
			{
				len++;
			}
			if ( len > c->unique_len )
			{
				c->unique_len = len;
			}
		}
		if ( c->children )
		{
			build_shortest ( c->children );
		}
	}
	return LIBCLI::OK;
}

int
CLI::set_privilege
(
        int priv
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int old = this->privilege;
	this->privilege = priv;
	if ( priv != old )
	{
		set_promptchar ( priv == LIBCLI::PRIVILEGED ? "# " : "> " );
		build_shortest ( this->commands );
	}
	return old;
}

void
CLI::set_modestring
(
        const char* modestring
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	free_z ( this->modestring );
	if ( modestring )
	{
		this->modestring = strdup ( modestring );
	}
}

int
CLI::set_configmode
(
        int mode,
        const char* config_desc
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int old = this->mode;
	this->mode = mode;
	if ( mode != old )
	{
		if ( !this->mode )
		{
			// Not config mode
			set_modestring ( NULL );
		}
		else if ( config_desc && *config_desc )
		{
			char str[64];
			snprintf ( str, sizeof ( str ), "(config-%s)", config_desc );
			set_modestring ( str );
		}
		else
		{
			set_modestring ( "(config)" );
		}
		build_shortest ( this->commands );
	}
	return old;
}

void
CLI::register_command
(
        Command<CLI>*   command,
        Command<CLI>*   parent
)
{
	DEBUG d ( CLI_TRACE );
	if ( ! command )
	{
		return;
	}
	if ( parent )
	{
		command->parent = parent;
		if ( ! parent->children )
		{
			parent->children = command;
			build_shortest ( command->parent );
			return;
		}
		Command<CLI>* c;
		for ( c = parent->children; c && c->next; c = c->next )
			/* intentionally empty */
		{
			;
		}
		if ( c )
		{
			c->next = command;
			build_shortest ( parent );
			return;
		}
		cout << "bummer!" << endl;
	}
	if ( ! this->commands )
	{
		this->commands = command;
	}
	else
	{
		Command<CLI>* c;
		for ( c = this->commands; c && c->next; c = c->next )
			/* intentionally empty */
		{
			;
		}
		if ( c )
		{
			c->next = command;
		}
		else
		{
			cout << "bummer 2" << endl;
		}
	}
	build_shortest ( ( command->parent ) ? command->parent : this->commands );
}

void
CLI::free_command
(
        Command<CLI> *cmd
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	Command<CLI> *c,*p;
	for ( c = cmd->children; c; )
	{
		p = c->next;
		free_command ( c );
		c = p;
	}
	free ( cmd->command );
	if ( cmd->help )
	{
		free ( cmd->help );
	}
	free ( cmd );
}

int
CLI::unregister_command
(
        char* command
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	Command<CLI> *c, *p = NULL;
	if ( ! command )
	{
		return -1;
	}
	if ( !this->commands )
	{
		return LIBCLI::OK;
	}
	for ( c = this->commands; c; c = c->next )
	{
		if ( strcmp ( c->command, command ) == 0 )
		{
			if ( p )
			{
				p->next = c->next;
			}
			else
			{
				this->commands = c->next;
			}
			free_command ( c );
			return LIBCLI::OK;
		}
		p = c;
	}
	return LIBCLI::OK;
}

int
CLI::show_help
(
        Command<CLI> *c
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	Command<CLI> *p;
	for ( p = c; p; p = p->next )
	{
		if ( p->command
		                && ( this->privilege >= p->privilege )
		                && ( ( p->mode == this->mode ) || ( p->mode == MODE_ANY ) ) )
		{
			error ( "  %-20s %s", p->command, p->help ? : "" );
		}
	}
	return LIBCLI::OK;
}

int
CLI::internal_enable
(
        UNUSED ( char* command ),
        UNUSED ( char* argv[] ),
        UNUSED ( int argc )
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	if ( this->privilege == LIBCLI::PRIVILEGED )
	{
		return LIBCLI::OK;
	}
	if ( !this->enable_password && !this->enable_callback && !this->cpp_enable_callback)
	{
		/* no password required, set privilege immediately */
		set_privilege ( LIBCLI::PRIVILEGED );
		set_configmode ( LIBCLI::MODE_EXEC, NULL );
	}
	else
	{
		/* require password entry */
		this->state = LIBCLI::STATE_ENABLE_PASSWORD;
	}
	return LIBCLI::OK;
}

int
CLI::internal_disable
(
        UNUSED ( char* command ),
        UNUSED ( char* argv[] ),
        UNUSED ( int argc )
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	set_privilege ( LIBCLI::UNPRIVILEGED );
	set_configmode ( LIBCLI::MODE_EXEC, NULL );
	return LIBCLI::OK;
}

int
CLI::internal_help
(
        UNUSED ( char* command ),
        UNUSED ( char* argv[] ),
        UNUSED ( int argc )
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	print (
		"Help may be requested at any point in a command by entering\n"
		"a question mark '?'.  If nothing matches, the help list will\n"
		"be empty and you must backup until entering a '?' shows the\n"
		"available options.\n"
		"Two styles of help are provided:\n"
		"1. Full help is available when you are ready to enter a\n"
		"   command argument (e.g. 'show ?') and describes each possible\n"
		"   argument.\n"
		"2. Partial help is provided when an abbreviated argument is entered\n"
		"   and you want to know what arguments match the input\n"
		"   (e.g. 'show pr?'.)\n"
	);
	// show_help ( this->commands );
	return LIBCLI::OK;
}

int
CLI::internal_history
(
        UNUSED ( char* command ),
        UNUSED ( char* argv[] ),
        UNUSED ( int argc )
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int i;
	error ( "\nCommand history:" );
	for ( i = 0; i < LIBCLI::MAX_HISTORY; i++ )
	{
		if ( this->history[i] )
		{
			error ( "%3d. %s", i, this->history[i] );
		}
	}
	return LIBCLI::OK;
}

int
CLI::internal_quit
(
        UNUSED ( char* command ),
        UNUSED ( char* argv[] ),
        UNUSED ( int argc )
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	set_privilege ( LIBCLI::UNPRIVILEGED );
	set_configmode ( LIBCLI::MODE_EXEC, NULL );
	return LIBCLI::QUIT;
}

int
CLI::internal_exit
(
        char* command,
        char* argv[],
        int argc
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	if ( this->mode == MODE_EXEC )
	{
		return internal_quit ( command, argv, argc );
	}
	if ( this->mode > MODE_CONFIG )
	{
		set_configmode ( MODE_CONFIG, NULL );
	}
	else
	{
		set_configmode ( MODE_EXEC, NULL );
	}
	this->service = NULL;
	return LIBCLI::OK;
}

int
CLI::int_configure_terminal
(
        UNUSED ( char* command ),
        UNUSED ( char* argv[] ),
        UNUSED ( int argc )
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	set_configmode ( MODE_CONFIG, NULL );
	return LIBCLI::OK;
}

CLI::CLI
()
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	Command<CLI> *c;
	this->modestring	= 0;
	this->banner	= 0;
	this->promptchar	= 0;
	this->hostname	= 0;
	this->buffer	= 0;
	this->enable_password	= 0;
	this->client	= 0;
	this->conn	= 0;
	this->service	= 0;
	this->users	= 0;
	this->commands	= 0;
	this->filters	= 0;
	this->auth_callback	= 0;
	this->cpp_auth_callback	= 0;
	this->regular_callback	= 0;
	this->enable_callback	= 0;
	this->cpp_enable_callback	= 0;
	this->print_callback	= 0;
	this->from_socket = false;
	int i;
	for ( i = 0; i < MAX_HISTORY; i++ )
	{
		this->history[i] = 0;
	}
	this->buf_size = 1024;
	if ( ! ( this->buffer = ( char* ) calloc ( this->buf_size, 1 ) ) )
	{
		throw mem_error ();
	}
	register_command ( new Command<CLI> (
	                           this,
	                           "help",
	                           & CLI::internal_help,
	                           LIBCLI::UNPRIVILEGED,
	                           LIBCLI::MODE_ANY,
	                           "show available commands"
	                   ) );
	register_command ( new Command<CLI> (
	                           this,
	                           "quit",
	                           & CLI::internal_quit,
	                           LIBCLI::UNPRIVILEGED,
	                           LIBCLI::MODE_ANY,
	                           "Disconnect"
	                   ) );
	register_command ( new Command<CLI> (
	                           this,
	                           "exit",
	                           & CLI::internal_exit,
	                           LIBCLI::UNPRIVILEGED,
	                           LIBCLI::MODE_ANY,
	                           "Exit from current mode"
	                   ) );
	register_command ( new Command<CLI> (
	                           this,
	                           "history",
	                           & CLI::internal_history,
	                           LIBCLI::UNPRIVILEGED,
	                           LIBCLI::MODE_ANY,
	                           "Show a list of previously run commands"
	                   ) );
	register_command ( new Command<CLI> (
	                           this,
	                           "enable",
	                           & CLI::internal_enable,
	                           LIBCLI::UNPRIVILEGED,
	                           LIBCLI::MODE_EXEC,
	                           "Turn on privileged commands"
	                   ) );
	register_command ( new Command<CLI> (
	                           this,
	                           "disable",
	                           & CLI::internal_disable,
	                           LIBCLI::PRIVILEGED,
	                           LIBCLI::MODE_EXEC,
	                           "Turn off privileged commands"
	                   ) );
	c = new Command<CLI> (
	        this,
	        "configure",
	        LIBCLI::PRIVILEGED,
	        LIBCLI::MODE_EXEC,
	        "Enter configuration mode"
	);
	register_command ( c );
	register_command ( new Command<CLI> (
	                           this,
	                           "terminal",
	                           & CLI::int_configure_terminal,
	                           LIBCLI::PRIVILEGED,
	                           LIBCLI::MODE_EXEC,
	                           "Configure from the terminal"
	                   ), c );
	this->privilege = this->mode = -1;
	set_privilege ( LIBCLI::UNPRIVILEGED );
	set_configmode ( LIBCLI::MODE_EXEC, 0 );
}

void
CLI::destroy
()
{
	if ( this->from_socket == false )  // read from stdin
	{
#ifndef _MSC_VER
		( void ) tcsetattr ( fileno ( stdin ), TCSANOW, &OldModes );
#endif
	}
}

void
CLI::unregister_all
(
        Command<CLI> *command
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	Command<CLI> *c, *p = NULL;
	if ( ! command )
	{
		command = this->commands;
	}
	if ( ! command )
	{
		return;
	}
	for ( c = command; c; )
	{
		p = c->next;
		// Unregister all child commands
		if ( c->children )
		{
			unregister_all ( c->children );
		}
		if ( c->command )
		{
			free ( c->command );
		}
		if ( c->help )
		{
			free ( c->help );
		}
		free ( c );
		c = p;
	}
}

int
CLI::done
()
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	unp* u = this->users, *n;
	free_history();
	// Free all users
	while ( u )
	{
		if ( u->username )
		{
			free ( u->username );
		}
		if ( u->password )
		{
			free ( u->password );
		}
		n = u->next;
		free ( u );
		u = n;
	}
	/* free all commands */
	unregister_all ( 0 );
	free_z ( this->modestring );
	free_z ( this->banner );
	free_z ( this->promptchar );
	free_z ( this->hostname );
	free_z ( this->buffer );
	return LIBCLI::OK;
}

int
CLI::add_history
(
        char* cmd
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int i;
	for ( i = 0; i < MAX_HISTORY; i++ )
	{
		if ( !this->history[i] )
		{
			if ( i == 0 || strcasecmp ( this->history[i-1], cmd ) )
				if ( ! ( this->history[i] = strdup ( cmd ) ) )
				{
					return LIBCLI::ERROR;
				}
			return LIBCLI::OK;
		}
	}
	// No space found, drop one off the beginning of the list
	free ( this->history[0] );
	for ( i = 0; i < MAX_HISTORY-1; i++ )
	{
		this->history[i] = this->history[i+1];
	}
	if ( ! ( this->history[MAX_HISTORY - 1] = strdup ( cmd ) ) )
	{
		return LIBCLI::ERROR;
	}
	return LIBCLI::OK;
}

void
CLI::free_history
()
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int i;
	for ( i = 0; i < MAX_HISTORY; i++ )
	{
		if ( this->history[i] )
		{
			free_z ( this->history[i] );
		}
	}
}

int
CLI::parse_line
(
        char* line,
        char* words[],
        int max_words
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int nwords = 0;
	char* p = line;
	char* word_start = 0;
	int inquote = 0;
	while ( *p )
	{
		if ( ! isspace ( *p ) )
		{
			word_start = p;
			break;
		}
		p++;
	}
	while ( nwords < max_words - 1 )
	{
		if ( !*p || *p == inquote
		                || ( word_start && !inquote && ( isspace ( *p ) || *p == '|' ) ) )
		{
			if ( word_start )
			{
				int len = p - word_start;
				memcpy ( words[nwords] = ( char* ) malloc ( len + 1 ), word_start, len );
				words[nwords++][len] = 0;
			}
			if ( !*p )
			{
				break;
			}
			if ( inquote )
			{
				p++;        /* skip over trailing quote */
			}
			inquote = 0;
			word_start = 0;
		}
		else if ( *p == '"' || *p == '\'' )
		{
			inquote = *p++;
			word_start = p;
		}
		else
		{
			if ( !word_start )
			{
				if ( *p == '|' )
				{
					if ( ! ( words[nwords++] = strdup ( "|" ) ) )
					{
						return 0;
					}
				}
				else if ( !isspace ( *p ) )
				{
					word_start = p;
				}
			}
			p++;
		}
	}
	words[nwords] = 0;
	return nwords;
}

char*
CLI::join_words
(
        int argc,
        char** argv
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	char* p;
	int len = 0;
	int i;
	for ( i = 0; i < argc; i++ )
	{
		if ( i )
		{
			len += 1;
		}
		len += strlen ( argv[i] );
	}
	p = ( char* ) malloc ( len + 1 );
	p[0] = 0;
	for ( i = 0; i < argc; i++ )
	{
		if ( i )
		{
			strcat ( p, " " );
		}
		strcat ( p, argv[i] );
	}
	return p;
}

int
CLI::find_command
(
        Command<CLI> *commands,
        int num_words,
        char* words[],
        int start_word,
        int filters[]
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	Command<CLI> *c, *again = NULL;
	int c_words = num_words;
	if ( filters[0] )
	{
		c_words = filters[0];
	}
	// Deal with ? for help
	if ( ! words[start_word] )
	{
		return LIBCLI::ERROR;
	}
	if ( words[start_word][strlen ( words[start_word] ) - 1] == '?' )
	{
		int l = strlen ( words[start_word] )-1;
		for ( c = commands; c; c = c->next )
		{
			if ( strncasecmp ( c->command, words[start_word], l ) == 0
			                && ( c->have_callback || c->children )
			                && this->privilege >= c->privilege
			                && ( c->mode == this->mode || c->mode == MODE_ANY ) )
			{
				error ( "  %-20s %s", c->command, c->help ? : "" );
			}
		}
		if ( commands->parent && commands->parent->have_callback )
		{
			error ( "  %-20s %s", "<br>",  commands->parent->help ? : "" );
		}
		return LIBCLI::OK;
	}
	for ( c = commands; c; c = c->next )
	{
		if ( this->privilege < c->privilege )
		{
			continue;
		}
		if ( strncasecmp ( c->command, words[start_word], c->unique_len ) )
		{
			continue;
		}
		if ( strncasecmp ( c->command, words[start_word], strlen ( words[start_word] ) ) )
		{
			continue;
		}
AGAIN:
		if ( c->mode == this->mode || c->mode == MODE_ANY )
		{
			int rc = LIBCLI::OK;
			int f;
			filter_t** filt = &this->filters;
			// Found a word!
			if ( ! c->children )
			{
				// Last word
				if ( ! c->have_callback )
				{
					error ( "No callback for \"%s\"", c->command );
					return LIBCLI::ERROR;
				}
			}
			else
			{
				if ( start_word == c_words - 1 )
				{
					if ( c->have_callback )
					{
						goto CORRECT_CHECKS;
					}
					error ( "Incomplete command" );
					return LIBCLI::ERROR;
				}
				rc = find_command ( c->children, num_words, words, start_word + 1, filters );
				if ( rc == LIBCLI::ERROR_ARG )
				{
					if ( c->have_callback )
					{
						rc = LIBCLI::OK;
						goto CORRECT_CHECKS;
					}
					else
					{
						error ( "Invalid %s \"%s\"", commands->parent ? "argument" : "command", words[start_word] );
					}
				}
				return rc;
			}
			if ( ! c->have_callback )
			{
				error ( "Internal server error processing \"%s\"", c->command );
				return LIBCLI::ERROR;
			}
CORRECT_CHECKS:
			for ( f = 0; rc == LIBCLI::OK && filters[f]; f++ )
			{
				int n = num_words;
				char** argv;
				int argc;
				int len;
				if ( filters[f+1] )
				{
					n = filters[f+1];
				}
				if ( filters[f] == n - 1 )
				{
					error ( "Missing filter" );
					return LIBCLI::ERROR;
				}
				argv = words + filters[f] + 1;
				argc = n - ( filters[f] + 1 );
				len = strlen ( argv[0] );
				if ( argv[argc - 1][strlen ( argv[argc - 1] ) - 1] == '?' )
				{
					if ( argc == 1 )
					{
						int i;
						for ( i = 0; filter_cmds[i].cmd; i++ )
						{
							error ( "  %-20s %s", filter_cmds[i].cmd, filter_cmds[i].help );
						}
					}
					else
					{
						if ( argv[0][0] != 'c' ) // count
						{
							error ( "  WORD" );
						}
						if ( argc > 2 || argv[0][0] == 'c' ) // count
						{
							error ( "  <cr>" );
						}
					}
					return LIBCLI::OK;
				}
				if ( argv[0][0] == 'b' && len < 3 ) // [beg]in, [bet]ween
				{
					error ( "Ambiguous filter \"%s\" (begin, between)", argv[0] );
					return LIBCLI::ERROR;
				}
				*filt = ( filter_t* ) calloc ( sizeof ( filter_t ), 1 );
				if ( !strncmp ( "include", argv[0], len )
				                ||  !strncmp ( "exclude", argv[0], len )
				                ||  !strncmp ( "grep", argv[0], len )
				                ||  !strncmp ( "egrep", argv[0], len ) )
				{
					rc = match_filter_init ( argc, argv, *filt );
				}
				else if ( !strncmp ( "begin", argv[0], len )
				                || !strncmp ( "between", argv[0], len ) )
				{
					rc = range_filter_init ( argc, argv, *filt );
				}
				else if ( !strncmp ( "count", argv[0], len ) )
				{
					rc = count_filter_init ( argc, argv, *filt );
				}
				else
				{
					error ( "Invalid filter \"%s\"", argv[0] );
					rc = LIBCLI::ERROR;
				}
				if ( rc == LIBCLI::OK )
				{
					filt = & ( *filt )->next;
				}
				else
				{
					free ( *filt );
					*filt = 0;
				}
			}
			if ( rc == LIBCLI::OK )
			{
				rc = c->exec ( c->command, words + start_word + 1, c_words - start_word - 1 );
			}
			while ( this->filters )
			{
				filter_t* filt = this->filters;
				// call one last time to clean up
				filt->exec ( *this, NULL );
				this->filters = filt->next;
				free ( filt );
			}
			return rc;
		}
		else if ( this->mode > MODE_CONFIG && c->mode == MODE_CONFIG )
		{
			// command matched but from another mode,
			// remember it if we fail to find correct command
			again = c;
		}
	}
	// drop out of config submode if we have matched command on MODE_CONFIG
	if ( again )
	{
		c = again;
		set_configmode ( MODE_CONFIG, NULL );
		goto AGAIN;
	}
	if ( start_word == 0 )
	{
		error ( "Invalid %s \"%s\"", commands->parent ? "argument" : "command", words[start_word] );
	}
	return LIBCLI::ERROR_ARG;
}

int
CLI::run_command
(
        char* command
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int r;
	unsigned int num_words, i, f;
	char* words[128] = {0};
	int filters[128] = {0};
	if ( ! command )
	{
		return LIBCLI::ERROR;
	}
	while ( isspace ( *command ) )
	{
		command++;
	}
	if ( ! *command )
	{
		return LIBCLI::OK;
	}
	num_words = parse_line ( command, words, sizeof ( words ) / sizeof ( words[0] ) );
	for ( i = f = 0; i < num_words && f < sizeof ( filters ) / sizeof ( filters[0] ) - 1; i++ )
	{
		if ( words[i][0] == '|' )
		{
			filters[f++] = i;
		}
	}
	filters[f] = 0;
	if ( num_words )
	{
		r = find_command ( this->commands, num_words, words, 0, filters );
	}
	else
	{
		r = LIBCLI::ERROR;
	}
	for ( i = 0; i < num_words; i++ )
	{
		free ( words[i] );
	}
	if ( r == LIBCLI::QUIT )
	{
		return r;
	}
	return LIBCLI::OK;
}

int
CLI::get_completions
(
        char* command,
        char** completions,
        int max_completions
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	Command<CLI> *c;
	Command<CLI> *n;
	Command<CLI> *p = 0;
	int num_words, i, k=0, j;
	int num_words_line;
	char* words[128] = {0};
	int filter = 0;
	if ( ! command )
	{
		return 0;
	}
	while ( isspace ( *command ) )
	{
		command++;
	}
	num_words_line = parse_line ( command, words, sizeof ( words ) /sizeof ( words[0] ) );
	num_words = num_words_line;
	if ( !command[0] || command[strlen ( command )-1] == ' ' )
	{
		num_words++;
	}
	if ( ! num_words )
	{
		return 0;
	}
	for ( i = 0; i < num_words; i++ )
	{
		if ( words[i] && words[i][0] == '|' )
		{
			filter = i;
		}
	}
	if ( filter ) // complete filters
	{
		unsigned len = 0;
		if ( filter < num_words - 1 ) // filter already completed
		{
			return 0;
		}
		if ( filter == num_words - 1 )
		{
			len = strlen ( words[num_words-1] );
		}
		for ( i = 0; filter_cmds[i].cmd && k < max_completions; i++ )
		{
			if ( !len || ( len < strlen ( filter_cmds[i].cmd )
			                && !strncmp ( filter_cmds[i].cmd, words[num_words - 1], len ) ) )
			{
				completions[k++] = ( char* ) filter_cmds[i].cmd;
			}
		}
		completions[k] = NULL;
		return k;
	}
	j = 0;
	for ( c = this->commands, i = 0; c && i < num_words && k < max_completions; c = n )
	{
		n = c->next;
		if ( this->privilege < c->privilege )
		{
			continue;
		}
		if ( c->mode != this->mode && c->mode != MODE_ANY )
		{
			continue;
		}
		if ( words[i] && strncasecmp ( c->command, words[i], strlen ( words[i] ) ) )
		{
			continue;
		}
		if ( i < num_words - 1 )
		{
			if ( strlen ( words[i] ) < c->unique_len )
			{
				continue;
			}
			j = 0;
			p = c;
			n = c->children;
			i++;
			continue;
		}
		if ( ( (j > 0)  || c->next ) || ( ( p != 0 ) && ( p->have_callback ) ) )
		{	// more than one completion
			if ( j == 0 )
			{
				error ( " " );
				j++;
			}
			print ( "  %-20s %s", c->command, c->help ? : "" );
		}
		if (strncmp (command, c->command, strlen (c->command)) != 0)
			completions[k++] = c->command;
	}
	if (completions[k-1])
	if ( p != 0 )
	if ( k != 0 )
	if (strncmp (words[num_words_line - 1], completions[k-1], strlen (words[num_words_line - 1])) != 0)
	{
		if ( p->have_callback )
		{
			if ( j == 0 )
			{
				error ( " " );
			}
			print ( "  %-20s %s", "<br>", p->help ? : "" );
			k++;
		}
	}
	return k;
}

void
CLI::clear_line
(
        int sockfd,
        char* cmd,
        int l,
        int cursor
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int i;
	if ( cursor < l )
	{
		for ( i = 0; i < ( l - cursor ); i++ ) write ( sockfd, " ", 1 )
			/* intentionally empty */ ;
	}
	for ( i = 0; i < l; i++ )
	{
		cmd[i] = '\b';
	}
	for ( ; i < l * 2; i++ )
	{
		cmd[i] = ' ';
	}
	for ( ; i < l * 3; i++ )
	{
		cmd[i] = '\b';
	}
	write ( sockfd, cmd, i );
	memset ( cmd, 0, i );
	l = cursor = 0;
}

void
CLI::reprompt
()
{
	this->showprompt = 1;
}

void
CLI::regular
(
        int ( *callback ) ()
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	this->regular_callback = callback;
}

#define DES_PREFIX "{crypt}"    /* to distinguish clear text from DES crypted */
#define MD5_PREFIX "$1$"

int
CLI::pass_matches
(
        char* pass,
        char* tried_pass
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int des;
	if ( ( des = !strncasecmp ( pass, DES_PREFIX, sizeof ( DES_PREFIX )-1 ) ) )
	{
		pass += sizeof ( DES_PREFIX )-1;
	}
	if ( des || !strncmp ( pass, MD5_PREFIX, sizeof ( MD5_PREFIX )-1 ) )
	{
		tried_pass = crypt ( tried_pass, pass );
	}
	return !strcmp ( pass, tried_pass );
}

int
CLI::show_prompt
(
        int sockfd
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int len = 0;
	if ( this->hostname )
	{
		len += write ( sockfd, this->hostname, strlen ( this->hostname ) );
	}
	if ( this->modestring )
	{
		len += write ( sockfd, this->modestring, strlen ( this->modestring ) );
	}
	return len + write ( sockfd, this->promptchar, strlen ( this->promptchar ) );
}

void
CLI::setup_terminal ()
{
	// I believe 'termios' is not present on windows!
#ifndef _MSC_VER
	struct termios NewModes;
	setbuf ( stdin, ( char* ) 0 );
	(void) tcgetattr (fileno (stdin), &OldModes);
	NewModes = OldModes;
	NewModes.c_lflag &= ~ ( ICANON );
	NewModes.c_lflag &= ~ ( ECHO | ECHOE | ECHOK );
	NewModes.c_lflag |= ECHONL;
	( void ) tcsetattr ( fileno ( stdin ), TCSANOW, &NewModes );
#endif
}

int
CLI::loop
(
        int sockfd
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	unsigned char c;
	int n, l, oldl = 0, is_telnet_option = 0, skip = 0, esc = 0;
	int cursor = 0, insertmode = 1;
	char* cmd = NULL, *oldcmd = 0;
	char* username = NULL, *password = NULL;
	const char* negotiate =
	        "\xFF\xFB\x03"
	        "\xFF\xFB\x01"
	        "\xFF\xFD\x03"
	        "\xFF\xFD\x01";
	this->state = STATE_LOGIN;
	free_history ();
	if ( sockfd == fileno ( stdin ) )  // read from stdin
	{
		this->from_socket = false;
		this->setup_terminal ();
	}
	else    // read from socket
	{
		this->from_socket = true;
		write ( sockfd, negotiate, strlen ( negotiate ) );
	}
	if ( ( cmd = ( char* ) malloc ( 4096 ) ) == NULL )
	{
		return LIBCLI::ERROR;
	}
	if ( ! ( this->client = fdopen ( sockfd, "w+" ) ) )
	{
		return LIBCLI::ERROR;
	}
	setbuf ( this->client, NULL );
	if ( this->banner )
	{
		error ( "%s", this->banner );
	}
	/* start off in unprivileged mode */
	set_privilege ( LIBCLI::UNPRIVILEGED );
	set_configmode ( LIBCLI::MODE_EXEC, NULL );
	/* no auth required? */
	if ( !this->users && !this->auth_callback && !this->cpp_auth_callback )
	{
		this->state = STATE_NORMAL;
	}
	while ( 1 )
	{
		signed int in_history = 0;
		struct timeval tm;
		this->showprompt = 1;
		if ( oldcmd )
		{
			l = cursor = oldl;
			oldcmd[l] = 0;
			this->showprompt = 1;
			oldcmd = NULL;
			oldl = 0;
		}
		else
		{
			memset ( cmd, 0, 4096 );
			l = 0;
			cursor = 0;
		}
		tm.tv_sec = 1;
		tm.tv_usec = 0;
		while ( 1 )
		{
			int sr;
			fd_set r;
			if ( this->showprompt )
			{
				if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
				{
					write ( sockfd, "\r\n", 2 );
				}
				switch ( this->state )
				{
				case STATE_LOGIN:
					write ( sockfd, "Username: ", strlen ( "Username: " ) );
					break;
				case STATE_PASSWORD:
					write ( sockfd, "Password: ", strlen ( "Password: " ) );
					break;
				case STATE_NORMAL:
				case STATE_ENABLE:
					show_prompt ( sockfd );
					write ( sockfd, cmd, l );
					if ( cursor < l )
					{
						int n = l - cursor;
						while ( n-- )
						{
							write ( sockfd, "\b", 1 );
						}
					}
					break;
				case STATE_ENABLE_PASSWORD:
					write ( sockfd, "Password: ", strlen ( "Password: " ) );
					break;
				}
				this->showprompt = 0;
			}
			FD_ZERO ( &r );
			FD_SET ( sockfd, &r );
			if ( ( sr = select ( sockfd + 1, &r, NULL, NULL, &tm ) ) < 0 )
			{
				/* select error */
				if ( errno == EINTR )
				{
					continue;
				}
				perror ( "read" );
				l = -1;
				break;
			}
			if ( sr == 0 )
			{
				/* timeout every second */
				if ( this->regular_callback && this->regular_callback() != LIBCLI::OK )
				{
					break;
				}
				tm.tv_sec = 1;
				tm.tv_usec = 0;
				continue;
			}
			if ( ( n = read ( sockfd, &c, 1 ) ) < 0 )
			{
				if ( errno == EINTR )
				{
					continue;
				}
				perror ( "read" );
				l = -1;
				break;
			}
			if ( n == 0 )
			{
				l = -1;
				break;
			}
			if ( skip )
			{
				skip--;
				continue;
			}
			if ( c == 255 && !is_telnet_option )
			{
				is_telnet_option++;
				continue;
			}
			if ( is_telnet_option )
			{
				if ( c >= 251 && c <= 254 )
				{
					is_telnet_option = c;
					continue;
				}
				if ( c != 255 )
				{
					is_telnet_option = 0;
					continue;
				}
				is_telnet_option = 0;
			}
			/* handle ANSI arrows */
			if ( esc )
			{
				if ( esc == '[' )
				{
					/* remap to readline control codes */
					switch ( c )
					{
					case 'A': /* Up */
						c = CTRL ( 'P' );
						break;
					case 'B': /* Down */
						c = CTRL ( 'N' );
						break;
					case 'C': /* Right */
						c = CTRL ( 'F' );
						break;
					case 'D': /* Left */
						c = CTRL ( 'B' );
						break;
					default:
						c = 0;
					}
					esc = 0;
				}
				else
				{
					esc = ( c == '[' ) ? c : 0;
					continue;
				}
			}
			if ( ( c == '\n' ) && ( sockfd == 0 ) )
			{
				if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
				{
					write ( sockfd, "\r\n", 2 );
				}
				break;
			}
			if ( ( c == 0 ) || ( c == '\n' ) )
			{
				continue;
			}
			if ( c == '\r' )
			{
				if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
				{
					write ( sockfd, "\r\n", 2 );
				}
				break;
			}
			if ( c == 27 )
			{
				esc = 1;
				continue;
			}
			if ( c == CTRL ( 'C' ) )
			{
				write ( sockfd, "\a", 1 );
				continue;
			}
			/* back word, backspace/delete */
			if ( c == CTRL ( 'W' ) || c == CTRL ( 'H' ) || c == 0x7f )
			{
				int back = 0;
				if ( c == CTRL ( 'W' ) ) /* word */
				{
					int nc = cursor;
					if ( l == 0 || cursor == 0 )
					{
						continue;
					}
					while ( nc && cmd[nc - 1] == ' ' )
					{
						nc--;
						back++;
					}
					while ( nc && cmd[nc - 1] != ' ' )
					{
						nc--;
						back++;
					}
				}
				else /* char */
				{
					if ( l == 0 || cursor == 0 )
					{
						write ( sockfd, "\a", 1 );
						continue;
					}
					back = 1;
				}
				if ( back )
				{
					while ( back-- )
					{
						if ( l == cursor )
						{
							cmd[--cursor] = 0;
							if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
							{
								write ( sockfd, "\b \b", 3 );
							}
						}
						else
						{
							int i;
							cursor--;
							if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
							{
								for ( i = cursor; i <= l; i++ ) cmd[i] = cmd[i+1]
									                /* intentionally empty */;
								write ( sockfd, "\b", 1 );
								write ( sockfd, cmd + cursor, strlen ( cmd + cursor ) );
								write ( sockfd, " ", 1 );
								for ( i = 0; i <= ( int ) strlen ( cmd + cursor ); i++ )
								{
									write ( sockfd, "\b", 1 );
								}
							}
						}
						l--;
					}
					continue;
				}
			}
			/* redraw */
			if ( c == CTRL ( 'L' ) )
			{
				int i;
				int cursorback = l - cursor;
				if ( this->state == STATE_PASSWORD || this->state == STATE_ENABLE_PASSWORD )
				{
					continue;
				}
				write ( sockfd, "\r\n", 2 );
				show_prompt ( sockfd );
				write ( sockfd, cmd, l );
				for ( i = 0; i < cursorback; i++ )
				{
					write ( sockfd, "\b", 1 );
				}
				continue;
			}
			/* clear line */
			if ( c == CTRL ( 'U' ) )
			{
				if ( this->state == STATE_PASSWORD || this->state == STATE_ENABLE_PASSWORD )
				{
					memset ( cmd, 0, l );
				}
				else
				{
					clear_line ( sockfd, cmd, l, cursor );
				}
				l = cursor = 0;
				continue;
			}
			/* kill to EOL */
			if ( c == CTRL ( 'K' ) )
			{
				if ( cursor == l )
				{
					continue;
				}
				if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
				{
					int c;
					for ( c = cursor; c < l; c++ )
					{
						write ( sockfd, " ", 1 );
					}
					for ( c = cursor; c < l; c++ )
					{
						write ( sockfd, "\b", 1 );
					}
				}
				memset ( cmd + cursor, 0, l - cursor );
				l = cursor;
				continue;
			}
			/* EOT */
			if ( c == CTRL ( 'D' ) )
			{
				if ( this->state == STATE_PASSWORD || this->state == STATE_ENABLE_PASSWORD )
				{
					break;
				}
				if ( l )
				{
					continue;
				}
				strcpy ( cmd, "quit" );
				l = cursor = strlen ( cmd );
				write ( sockfd, "quit\r\n", l + 2 );
				break;
			}
			/* disable */
			if ( c == CTRL ( 'Z' ) )
			{
				if ( this->mode != MODE_EXEC )
				{
					clear_line ( sockfd, cmd, l, cursor );
					set_configmode ( MODE_EXEC, NULL );
					this->showprompt = 1;
				}
				continue;
			}
			/* TAB completion */
			if ( c == CTRL ( 'I' ) )
			{
				char* completions[128];
				int num_completions = 0;
				if ( this->state == STATE_PASSWORD || this->state == STATE_ENABLE_PASSWORD )
				{
					continue;
				}
				if ( cursor != l )
				{
					continue;
				}
				num_completions = get_completions ( cmd, completions, 128 );
				// if ( num_completions )
					this->showprompt = 1;
				if ( num_completions == 1 )
				{	// Single completion
					for ( ; l > 0; l--, cursor-- )
					{
						if ( cmd[l-1] == ' ' || cmd[l-1] == '|' )
						{
							break;
						}
						write ( sockfd, "\b", 1 );
					}
					strcpy ( ( cmd + l ), completions[0] );
					l += strlen ( completions[0] );
					cmd[l++] = ' ';
					cursor = l;
				}
				continue;
			}
			/* history */
			if ( c == CTRL ( 'P' ) || c == CTRL ( 'N' ) )
			{
				int history_found = 0;
				if ( this->state == STATE_PASSWORD || this->state == STATE_ENABLE_PASSWORD )
				{
					continue;
				}
				if ( c == CTRL ( 'P' ) ) // Up
				{
					in_history--;
					if ( in_history < 0 )
					{
						for ( in_history = MAX_HISTORY-1; in_history >= 0; in_history-- )
						{
							if ( this->history[in_history] )
							{
								history_found = 1;
								break;
							}
						}
					}
					else
					{
						if ( this->history[in_history] )
						{
							history_found = 1;
						}
					}
				}
				else // Down
				{
					in_history++;
					if ( in_history >= MAX_HISTORY || !this->history[in_history] )
					{
						int i = 0;
						for ( i = 0; i < MAX_HISTORY; i++ )
						{
							if ( this->history[i] )
							{
								in_history = i;
								history_found = 1;
								break;
							}
						}
					}
					else
					{
						if ( this->history[in_history] )
						{
							history_found = 1;
						}
					}
				}
				if ( history_found && this->history[in_history] )
				{
					// Show history item
					clear_line ( sockfd, cmd, l, cursor );
					memset ( cmd, 0, 4096 );
					strncpy ( cmd, this->history[in_history], 4095 );
					l = cursor = strlen ( cmd );
					write ( sockfd, cmd, l );
				}
				continue;
			}
			/* left/right cursor motion */
			if ( c == CTRL ( 'B' ) || c == CTRL ( 'F' ) )
			{
				if ( c == CTRL ( 'B' ) ) /* Left */
				{
					if ( cursor )
					{
						if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
						{
							write ( sockfd, "\b", 1 );
						}
						cursor--;
					}
				}
				else /* Right */
				{
					if ( cursor < l )
					{
						if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
						{
							write ( sockfd, &cmd[cursor], 1 );
						}
						cursor++;
					}
				}
				continue;
			}
			/* start of line */
			if ( c == CTRL ( 'A' ) )
			{
				if ( cursor )
				{
					if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
					{
						write ( sockfd, "\r", 1 );
						show_prompt ( sockfd );
					}
					cursor = 0;
				}
				continue;
			}
			/* end of line */
			if ( c == CTRL ( 'E' ) )
			{
				if ( cursor < l )
				{
					if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
					{
						write ( sockfd, &cmd[cursor], l - cursor );
					}
					cursor = l;
				}
				continue;
			}
			/* normal character typed */
			if ( cursor == l )
			{
				/* append to end of line */
				cmd[cursor] = c;
				if ( l < 4095 )
				{
					l++;
					cursor++;
				}
				else
				{
					write ( sockfd, "\a", 1 );
					continue;
				}
			}
			else
			{
				// Middle of text
				if ( insertmode )
				{
					int i;
					// Move everything one character to the right
					if ( l >= 4094 )
					{
						l--;
					}
					for ( i = l; i >= cursor; i-- )
					{
						cmd[i + 1] = cmd[i];
					}
					// Write what we've just added
					cmd[cursor] = c;
					write ( sockfd, &cmd[cursor], l - cursor + 1 );
					for ( i = 0; i < ( l - cursor + 1 ); i++ )
					{
						write ( sockfd, "\b", 1 );
					}
					l++;
				}
				else
				{
					cmd[cursor] = c;
				}
				cursor++;
			}
			if ( this->state != STATE_PASSWORD && this->state != STATE_ENABLE_PASSWORD )
			{
				if ( c == '?' && cursor == l )
				{
					write ( sockfd, "\r\n", 2 );
					oldcmd = cmd;
					oldl = cursor = l - 1;
					break;
				}
				write ( sockfd, &c, 1 );
			}
			oldcmd = 0;
			oldl = 0;
			// lastchar = c;
		}
		if ( l < 0 )
		{
			break;
		}
		if ( !strcasecmp ( cmd, "quit" ) )
		{
			break;
		}
		if ( this->state == STATE_LOGIN )
		{
			if ( l == 0 )
			{
				continue;
			}
			/* require login */
			free_z ( username );
			if ( ! ( username = strdup ( cmd ) ) )
			{
				return 0;
			}
			this->state = STATE_PASSWORD;
			this->showprompt = 1;
		}
		else if ( this->state == STATE_PASSWORD )
		{
			/* require password */
			int allowed = 0;
			free_z ( password );
			if ( ! ( password = strdup ( cmd ) ) )
			{
				return 0;
			}
			if ( this->auth_callback )
			{
				if ( this->auth_callback ( username, password ) == LIBCLI::OK )
				{
					allowed++;
				}
			}
			else if ( this->cpp_auth_callback )
			{
				if ( CALL_MEMBER_FN ((*this), cpp_auth_callback)  ( username, password ) == LIBCLI::OK )
				{
					allowed++;
				}
			}
			if ( ! allowed )
			{
				unp* u;
				for ( u = this->users; u; u = u->next )
				{
					if ( !strcmp ( u->username, username ) && pass_matches ( u->password, password ) )
					{
						allowed++;
						break;
					}
				}
			}
			if ( allowed )
			{
				error ( "-ok-" );
				this->state = STATE_NORMAL;
				error ("type '?' or 'help' for help.");
			}
			else
			{
				error ( "\n\nAccess denied" );
				free_z ( username );
				free_z ( password );
				this->state = LIBCLI::STATE_LOGIN;
			}
			this->showprompt = 1;
		}
		else if ( this->state == LIBCLI::STATE_ENABLE_PASSWORD )
		{
			int allowed = 0;
			if ( this->enable_password )
			{
				/* check stored static enable password */
				if ( pass_matches ( this->enable_password, cmd ) )
				{
					allowed++;
				}
			}
			if ( !allowed )
			{
				/* check callback */
				if (this->enable_callback)
				{
					if ( this->enable_callback ( cmd ) )
					{
						allowed++;
					}
				}
				else if (this->cpp_enable_callback)
				{
					if ( CALL_MEMBER_FN ((*this), cpp_enable_callback) ( cmd ) )
					{
						allowed++;
					}
				}
			}
			if ( allowed )
			{
				this->state = LIBCLI::STATE_ENABLE;
				set_privilege ( LIBCLI::PRIVILEGED );
			}
			else
			{
				error ( "\n\nAccess denied" );
				this->state = LIBCLI::STATE_NORMAL;
			}
		}
		else
		{
			if ( l == 0 )
			{
				continue;
			}
			if ( cmd[l - 1] != '?' && strcasecmp ( cmd, "history" ) != 0 )
			{
				add_history ( cmd );
			}
			if ( run_command ( cmd ) == LIBCLI::QUIT )
			{
				break;
			}
		}
	}
	free_history ();
	free_z ( username );
	free_z ( password );
	free_z ( cmd );
	fclose ( this->client );
	this->client = 0;
	return LIBCLI::OK;
}

int
CLI::file
(
        FILE* fh,
        int privilege,
        int mode
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int oldpriv = set_privilege ( privilege );
	int oldmode = set_configmode ( mode, NULL );
	char buf[4096];
	while ( 1 )
	{
		char* p;
		char* cmd;
		char* end;
		if ( fgets ( buf, sizeof ( buf ), fh ) == NULL )
		{
			break;        /* end of file */
		}
		if ( ( p = strpbrk ( buf, "#\r\n" ) ) )
		{
			*p = 0;
		}
		cmd = buf;
		while ( isspace ( *cmd ) )
		{
			cmd++;
		}
		if ( !*cmd )
		{
			continue;
		}
		for ( p = end = cmd; *p; p++ )
		{
			if ( !isspace ( *p ) )
			{
				end = p;
			}
		}
		*++end = 0;
		if ( strcasecmp ( cmd, "quit" ) == 0 )
		{
			break;
		}
		if ( run_command ( cmd ) == LIBCLI::QUIT )
		{
			break;
		}
	}
	set_privilege ( oldpriv );
	set_configmode ( oldmode, NULL /* didn't save desc */ );
	return LIBCLI::OK;
}

void
CLI::_print
(
        int print_mode,
        const char* format,
        va_list ap
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	static char* buffer;
	static int size, len;
	char* p;
	int n;
	buffer = this->buffer;
	size = this->buf_size;
	len = strlen ( buffer );
	while ( ( n = vsnprintf ( buffer+len, size-len, format, ap ) ) >= size-len )
	{
		if ( ! ( buffer = ( char* ) realloc ( buffer, size += 1024 ) ) )
		{
			return;
		}
		this->buffer = buffer;
		this->buf_size = size;
	}
	if ( n < 0 ) // vaprintf failed
	{
		return;
	}
	p = buffer;
	do
	{
		char* next = strchr ( p, '\n' );
		filter_t* f = ( print_mode&PRINT_FILTERED ) ? this->filters : 0;
		int print = 1;
		if ( next )
		{
			*next++ = 0;
		}
		else if ( print_mode&PRINT_BUFFERED )
		{
			break;
		}
		while ( print && f )
		{
			print = ( f->exec ( *this, p, f->data ) == LIBCLI::OK );
			f = f->next;
		}
		if ( print )
		{
			if ( this->print_callback )
			{
				this->print_callback ( p );
			}
			else if ( this->client )
			{
				fprintf ( this->client, "%s\r\n", p );
			}
		}
		p = next;
	}
	while ( p );
	if ( p && *p )
	{
		if ( p != buffer )
		{
			bcopy ( p, buffer, strlen ( p ) );
		}
	}
	else
	{
		*buffer = 0;
	}
}

void
CLI::bufprint
(
        const char* format,
        ...
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	va_list ap;
	va_start ( ap, format );
	_print ( PRINT_BUFFERED|PRINT_FILTERED, format, ap );
	va_end ( ap );
}

void
CLI::vabufprint
(
        const char* format,
        va_list ap
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	_print ( PRINT_BUFFERED, format, ap );
}

void
CLI::print
(
        const char* format,
        ...
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	va_list ap;
	va_start ( ap, format );
	_print ( PRINT_FILTERED, format, ap );
	va_end ( ap );
}

void
CLI::error
(
        const char* format,
        ...
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	va_list ap;
	va_start ( ap, format );
	_print ( PRINT_PLAIN, format, ap );
	va_end ( ap );
}

int
CLI::match_filter_init
(
        int argc,
        char** argv,
        filter_t* filt
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	match_filter_state* state;
	int rflags;
	int i;
	char* p;
	if ( argc < 2 )
	{
		if ( this->client )
		{
			fprintf ( this->client, "Match filter requires an argument\r\n" );
		}
		return LIBCLI::ERROR;
	}
	filt->filter = &CLI::match_filter;
	state = new match_filter_state;
	filt->data = state;
	if ( argv[0][0] == 'i' // include/exclude
	                ||  ( argv[0][0] == 'e' && argv[0][1] == 'x' ) )
	{
		if ( argv[0][0] == 'e' )
		{
			state->flags = MATCH_INVERT;
		}
		state->match.str = join_words ( argc-1, argv+1 );
		return LIBCLI::OK;
	}
	state->flags = MATCH_REGEX;
	// grep/egrep
	rflags = REG_NOSUB;
	if ( argv[0][0] == 'e' ) // egrep
	{
		rflags |= REG_EXTENDED;
	}
	i = 1;
	while ( i < argc - 1 && argv[i][0] == '-' && argv[i][1] )
	{
		int last = 0;
		p = &argv[i][1];
		if ( strspn ( p, "vie" ) != strlen ( p ) )
		{
			break;
		}
		while ( *p )
		{
			switch ( *p++ )
			{
			case 'v':
				state->flags |= MATCH_INVERT;
				break;
			case 'i':
				rflags |= REG_ICASE;
				break;
			case 'e':
				last++;
				break;
			}
		}
		i++;
		if ( last )
		{
			break;
		}
	}
	p = join_words ( argc-i, argv+i );
	if ( ( i = regcomp ( &state->match.re, p, rflags ) ) )
	{
		if ( this->client )
		{
			fprintf ( this->client, "Invalid pattern \"%s\"\r\n", p );
		}
		free_z ( p );
		return LIBCLI::ERROR;
	}
	free_z ( p );
	return LIBCLI::OK;
}

int
CLI::match_filter
(
        char* cmd,
        void* data
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	match_filter_state* state = reinterpret_cast<match_filter_state*> ( data );
	int r = LIBCLI::ERROR;
	if ( !cmd ) // clean up
	{
		if ( state->flags & MATCH_REGEX )
		{
			regfree ( &state->match.re );
		}
		else
		{
			free ( state->match.str );
		}
		free ( state );
		return LIBCLI::OK;
	}
	if ( state->flags & MATCH_REGEX )
	{
		if ( !regexec ( &state->match.re, cmd, 0, NULL, 0 ) )
		{
			r = LIBCLI::OK;
		}
	}
	else
	{
		if ( strstr ( cmd, state->match.str ) )
		{
			r = LIBCLI::OK;
		}
	}
	if ( state->flags & MATCH_INVERT )
	{
		if ( r == LIBCLI::OK )
		{
			r = LIBCLI::ERROR;
		}
		else
		{
			r = LIBCLI::OK;
		}
	}
	return r;
}

int
CLI::range_filter_init
(
        int argc,
        char** argv,
        filter_t* filt
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	range_filter_state* state;
	char* from = 0;
	char* to = 0;
	if ( !strncmp ( argv[0], "bet", 3 ) ) // between
	{
		if ( argc < 3 )
		{
			if ( this->client )
			{
				fprintf ( this->client, "Between filter requires 2 arguments\r\n" );
			}
			return LIBCLI::ERROR;
		}
		if ( ! ( from = strdup ( argv[1] ) ) )
		{
			return LIBCLI::ERROR;
		}
		to = join_words ( argc-2, argv+2 );
	}
	else // begin
	{
		if ( argc < 2 )
		{
			if ( this->client )
			{
				fprintf ( this->client, "Begin filter requires an argument\r\n" );
			}
			return LIBCLI::ERROR;
		}
		from = join_words ( argc-1, argv+1 );
	}
	filt->filter = &CLI::range_filter;
	state = new range_filter_state;
	filt->data = state;
	state->from = from;
	state->to = to;
	return LIBCLI::OK;
}

int
CLI::range_filter
(
        char* cmd,
        void* data
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	range_filter_state* state = ( range_filter_state* ) data;
	int r = LIBCLI::ERROR;
	if ( !cmd ) // clean up
	{
		free_z ( state->from );
		free_z ( state->to );
		free_z ( state );
		return LIBCLI::OK;
	}
	if ( !state->matched )
	{
		state->matched = !!strstr ( cmd, state->from );
	}
	if ( state->matched )
	{
		r = LIBCLI::OK;
		if ( state->to && strstr ( cmd, state->to ) )
		{
			state->matched = 0;
		}
	}
	return r;
}

int
CLI::count_filter_init
(
        int argc,
        UNUSED ( char** argv ),
        filter_t* filt
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	if ( argc > 1 )
	{
		if ( this->client )
		{
			fprintf ( this->client, "Count filter does not take arguments\r\n" );
		}
		return LIBCLI::ERROR;
	}
	filt->filter = &CLI::count_filter;
	filt->data = new int[1];
	if ( ! filt->data )
	{
		return LIBCLI::ERROR;
	}
	return LIBCLI::OK;
}

int
CLI::count_filter
(
        char* cmd,
        void* data
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	int* count = ( int* ) data;
	if ( !cmd ) // clean up
	{
		// print count
		if ( this->client )
		{
			fprintf ( this->client, "%d\r\n", *count );
		}
		free ( count );
		return LIBCLI::OK;
	}
	while ( isspace ( *cmd ) )
	{
		cmd++;
	}
	if ( *cmd )
	{
		( *count ) ++;        // only count non-blank lines
	}
	return LIBCLI::ERROR; // no output
}

void
CLI::set_print_callback
(
        void ( *callback ) ( char* )
)
{
	DEBUG d ( __FUNCTION__,__FILE__,__LINE__ );
	this->print_callback = callback;
}

}; // namespace LIBCLI
