/* Copyright 2001,2002 Roger Dingledine, Matej Pfajfar. */
/* See LICENSE for licensing information */
/* $Id$ */

/**
 * config.c 
 * Routines for loading the configuration file.
 *
 * Matej Pfajfar <mp292@cam.ac.uk>
 */

#include "or.h"
#include <string.h>

const char * 
basename(const char *filename)
{
  char *result;
  /* XXX This won't work on windows. */
  result = strrchr(filename, '/');
  if (result)
    return result;
  else
    return filename;
}

/* loads the configuration file */
int getconfig(char *conf_filename, config_opt_t *options)
{
  FILE *cf = NULL;
  int retval = 0;
  
  if ((!conf_filename) || (!options))
    return -1;
  
  /* load config file */
  cf = open_config(conf_filename);
  if (!cf)
  {
    log(LOG_ERR,"Could not open configuration file %s.",conf_filename);
    return -1;
  }
  retval = parse_config(cf,options);
  if (retval)
    return -1;

  return 0;
}

int getoptions(int argc, char **argv, or_options_t *options)
/**

A replacement for getargs() and getconfig() which uses the <popt> library to parse
both command-line arguments and configuration files. A specific configuration file
may be specified using the --ConfigFile option. If one is not specified, then the
configuration files at /etc/<cmd>rc and ~/.<cmd>rc will be loaded in that order so
user preferences will override the ones specified in /etc.

The --ConfigFile (-f) option may only be used on the command-line. All other command-line
options may also be specified in configuration files. <popt> aliases are enabled
so a user can define their own options in the /etc/popt or ~/.popt files as outlined
in "man popt" pages.

RETURN VALUE: 0 on success, non-zero on error
**/
{
   char *ConfigFile;
   int Verbose;
   int code;
   poptContext optCon;
   const char *cmd;
   struct poptOption opt_tab[] =
   {
      { "APPort",          'a',  POPT_ARG_INT,     &options->APPort,
         0, "application proxy port",                          "<port>" },
      { "CoinWeight",      'w',  POPT_ARG_FLOAT,   &options->CoinWeight,
         0, "coin weight used in determining routes",          "<weight>" },
      { "ConfigFile",      'f',  POPT_ARG_STRING,  &ConfigFile,
         0, "user specified configuration file",               "<file>" },
      { "LogLevel",        'l',  POPT_ARG_STRING,  &options->LogLevel,
         0, "emerg|alert|crit|err|warning|notice|info|debug",  "<level>" },
      { "MaxConn",         'm',  POPT_ARG_INT,     &options->MaxConn,
         0, "maximum number of incoming connections",          "<max>" },
      { "OPPort",          'o',  POPT_ARG_INT,     &options->OPPort,
         0, "onion proxy port",                                "<port>" },
      { "ORPort",          'p',  POPT_ARG_INT,     &options->ORPort,
         0, "onion router port",                               "<port>" },
      { "DirPort",         'd',  POPT_ARG_INT,     &options->DirPort,
         0, "directory server port",                           "<port>" },
      { "PrivateKeyFile",  'k',  POPT_ARG_STRING,  &options->PrivateKeyFile,
         0, "maximum number of incoming connections",          "<file>" },
      { "RouterFile",      'r',  POPT_ARG_STRING,  &options->RouterFile,
         0, "local port on which the onion proxy is running",  "<file>" },
      { "TrafficShaping",  't',  POPT_ARG_INT,     &options->TrafficShaping,
         0, "which traffic shaping policy to use",             "<policy>" },
      { "LinkPadding",     'P',  POPT_ARG_INT,     &options->LinkPadding,
         0, "whether to use link padding",                     "<padding>" },
      { "DirRebuildPeriod",'D',  POPT_ARG_INT,     &options->DirRebuildPeriod,
         0, "how many seconds between directory rebuilds",     "<rebuildperiod>" },
      { "DirFetchPeriod",  'F',  POPT_ARG_INT,     &options->DirFetchPeriod,
         0, "how many seconds between directory fetches",     "<fetchperiod>" },
//      { "ReconnectPeriod", 'e',  POPT_ARG_INT,     &options->ReconnectPeriod,
//         0, "how many seconds between retrying all OR connections", "<reconnectperiod>" },
      { "Role",            'R',  POPT_ARG_INT,     &options->Role,
         0, "4-bit global role id",                            "<role>" },
      { "Verbose",         'v',  POPT_ARG_NONE,    &Verbose,
         0, "display options selected before execution",       NULL },
      POPT_AUTOHELP  /* handles --usage and --help automatically */
      POPT_TABLEEND  /* marks end of table */
   };
   cmd = basename(argv[0]);
   optCon = poptGetContext(cmd,argc,(const char **)argv,opt_tab,0);

   poptReadDefaultConfig(optCon,0);       /* read <popt> alias definitions */

   /* assign default option values */

   bzero(options,sizeof(or_options_t));
   options->LogLevel = "debug";
   options->loglevel = LOG_DEBUG;
   options->CoinWeight = 0.8;
   options->LinkPadding = 0;
   options->DirRebuildPeriod = 600;
   options->DirFetchPeriod = 6000;
//   options->ReconnectPeriod = 6001;
   options->Role = ROLE_OR_LISTEN | ROLE_OR_CONNECT_ALL | ROLE_OP_LISTEN | ROLE_AP_LISTEN |
                   ROLE_DIR_LISTEN | ROLE_DIR_SERVER;

   code = poptGetNextOpt(optCon);         /* first we handle command-line args */
   if ( code == -1 )
   {
      if ( ConfigFile )                   /* handle user-specified config file */
         code = poptReadOptions(optCon,ConfigFile);
      else                                /* load Default configuration files */
         code = poptReadDefaultOptions(cmd,optCon);
   }

   switch(code)                           /* error checking */
   {
   case INT_MIN:
      log(LOG_ERR, "%s: Unable to open configuration file.\n", ConfigFile);
      break;
   case -1:
      code = 0;
      break;
   default:
      poptPrintUsage(optCon, stderr, 0);
      log(LOG_ERR, "%s: %s\n", poptBadOption(optCon, POPT_BADOPTION_NOALIAS), poptStrerror(code));
      break;
   }

   poptFreeContext(optCon);

   if ( code ) return code;      /* return here if we encountered any problems */

   /* Display options upon user request */

   if ( Verbose )                      
   {
      printf("LogLevel=%s, Role=%d\n",
             options->LogLevel,
             options->Role);
      printf("RouterFile=%s, PrivateKeyFile=%s\n",
             options->RouterFile,
             options->PrivateKeyFile);
      printf("ORPort=%d, OPPort=%d, APPort=%d DirPort=%d\n",
             options->ORPort,options->OPPort,
             options->APPort,options->DirPort);
      printf("CoinWeight=%6.4f, MaxConn=%d, TrafficShaping=%d, LinkPadding=%d\n",
             options->CoinWeight,
             options->MaxConn,
             options->TrafficShaping,
             options->LinkPadding);
      printf("DirRebuildPeriod=%d, DirFetchPeriod=%d\n",
             options->DirRebuildPeriod,
             options->DirFetchPeriod);
   }

   /* Validate options */

   if ( options->LogLevel )
   {
      if (!strcmp(options->LogLevel,"emerg"))
         options->loglevel = LOG_EMERG;
      else if (!strcmp(options->LogLevel,"alert"))
         options->loglevel = LOG_ALERT;
      else if (!strcmp(options->LogLevel,"crit"))
         options->loglevel = LOG_CRIT;
      else if (!strcmp(options->LogLevel,"err"))
         options->loglevel = LOG_ERR;
      else if (!strcmp(options->LogLevel,"warning"))
         options->loglevel = LOG_WARNING;
      else if (!strcmp(options->LogLevel,"notice"))
         options->loglevel = LOG_NOTICE;
      else if (!strcmp(options->LogLevel,"info"))
         options->loglevel = LOG_INFO;
      else if (!strcmp(options->LogLevel,"debug"))
         options->loglevel = LOG_DEBUG;
      else
      {
         log(LOG_ERR,"LogLevel must be one of emerg|alert|crit|err|warning|notice|info|debug.");
         code = -1;
      }
   }

   if ( options->Role < 0 || options->Role > 63 )
   {
      log(LOG_ERR,"Role option must be an integer between 0 and 63 (inclusive).");
      code = -1;
   }

   if ( options->RouterFile == NULL )
   {
      log(LOG_ERR,"RouterFile option required, but not found.");
      code = -1;
   }

   if ( ROLE_IS_OR(options->Role) && options->PrivateKeyFile == NULL )
   {
      log(LOG_ERR,"PrivateKeyFile option required for OR, but not found.");
      code = -1;
   }

   if ( (options->Role & ROLE_OR_LISTEN) && options->ORPort < 1 )
   {
      log(LOG_ERR,"ORPort option required and must be a positive integer value.");
      code = -1;
   }

   if ( (options->Role & ROLE_OP_LISTEN) && options->OPPort < 1 )
   {
      log(LOG_ERR,"OPPort option required and must be a positive integer value.");
      code = -1;
   }

   if ( (options->Role & ROLE_AP_LISTEN) && options->APPort < 1 )
   {
      log(LOG_ERR,"APPort option required and must be a positive integer value.");
      code = -1;
   }

   if ( (options->Role & ROLE_DIR_LISTEN) && options->DirPort < 1 )
   {
      log(LOG_ERR,"DirPort option required and must be a positive integer value.");
      code = -1;
   }

   if ( (options->Role & ROLE_AP_LISTEN) &&
        (options->CoinWeight < 0.0 || options->CoinWeight >= 1.0) )
   {
      log(LOG_ERR,"CoinWeight option must be a value from 0.0 upto 1.0, but not including 1.0.");
      code = -1;
   }

   if ( options->MaxConn <= 0 )
   {
      log(LOG_ERR,"MaxConn option must be a non-zero positive integer.");
      code = -1;
   }

   if ( options->MaxConn >= MAXCONNECTIONS )
   {
      log(LOG_ERR,"MaxConn option must be less than %d.", MAXCONNECTIONS);
      code = -1;
   }

   if ( options->TrafficShaping != 0 && options->TrafficShaping != 1 )
   {
      log(LOG_ERR,"TrafficShaping option must be either 0 or 1.");
      code = -1;
   }

   if ( options->LinkPadding != 0 && options->LinkPadding != 1 )
   {
      log(LOG_ERR,"LinkPadding option must be either 0 or 1.");
      code = -1;
   }

   if ( options->DirRebuildPeriod < 1)
   {
      log(LOG_ERR,"DirRebuildPeriod option must be positive.");
      code = -1;
   }

   if ( options->DirFetchPeriod < 1)
   {
      log(LOG_ERR,"DirFetchPeriod option must be positive.");
      code = -1;
   }

   return code;
}

