/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: config.c 7477 2018-06-22 04:07:56Z baker $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.7  2018/05/03 11:32:33  alexander
 *     Adding fix to create PickIndexDir if specified and it doesn't exist
 *
 *     Revision 1.6  2008/03/28 18:20:31  paulf
 *     added in PickIndexDir option to specify where pick indexes get stuffed
 *
 *     Revision 1.5  2007/02/26 13:59:08  paulf
 *     no coda changes (option)
 *
 *     Revision 1.4  2006/09/20 22:44:07  dietz
 *     Modified to be able to process multiple "StaFile" commands for setting
 *     per-channel picking parameters.
 *
 *     Revision 1.3  2005/04/08 23:57:19  dietz
 *     Added new config command "GetLogo" so pick_ew can select which logos
 *     to process. If no "GetLogo" commands are included, the default behavior
 *     is to process all TYPE_TRACEBUF and TYPE_TRACEBUF2 messages in InRing.
 *
 *     Revision 1.2  2002/05/16 17:00:07  patton
 *     Made logit changes.
 *
 *     Revision 1.1  2000/02/14 19:06:49  lucky
 *     Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (_WINNT)
 #include <direct.h>
 #include <io.h>
#else
 #include <sys/stat.h>
 #include <unistd.h>
#endif

#include "earthworm.h"
#include "kom.h"
#include "transport.h"
#include "nn_pick_ew.h"

#define ncommand 8          /* Number of commands in the config file */

 /***********************************************************************
  *                              GetConfig()                            *
  *             Processes command file using kom.c functions.           *
  *               Returns -1 if any errors are encountered.             *
  ***********************************************************************/

int GetConfig( char *config_file, GPARM *Gparm )
{
   char     init[ncommand];     /* Flags, one for each command */
   int      nmiss;              /* Number of commands that were missed */
   int      nfiles;
   int      i;

/* Set to zero one init flag for each required command
   ***************************************************/
   for ( i = 0; i < ncommand; i++ ) init[i] = 0;
   Gparm->nGetLogo = 0;
   Gparm->GetLogo  = NULL;
   Gparm->PickIndexDir  = NULL;	/* optional directory for pick index placement */
   Gparm->NoCoda = 0;		/* off by default, always calculate coda's */
   Gparm->NoCodaHorizontal = 0;		/* off by default, always calculate coda's on any channel */
   Gparm->nStaFile = 0;
   Gparm->StaFile  = NULL;

/* Open the main configuration file
   ********************************/
   nfiles = k_open( config_file );
   if ( nfiles == 0 )
   {
      logit( "e", "pick_ew: Error opening configuration file <%s>\n",
               config_file );
      return -1;
   }

/* Process all nested configuration files
   **************************************/
   while ( nfiles > 0 )          /* While there are config files open */
   {
      while ( k_rd() )           /* Read next line from active file  */
      {
         int  success;
         char *com;
         char *str;

         com = k_str();          /* Get the first token from line */

         if ( !com ) continue;             /* Ignore blank lines */
         if ( com[0] == '#' ) continue;    /* Ignore comments */

/* Open another configuration file
   *******************************/
         if ( com[0] == '@' )
         {
            success = nfiles + 1;
            nfiles  = k_open( &com[1] );
            if ( nfiles != success )
            {
               logit( "e", "pick_ew: Error opening command file <%s>.\n",
                        &com[1] );
               return -1;
            }
            continue;
         }

/* Read configuration parameters
   *****************************/
         else if ( k_its( "StaFile" ) )
         {
            str = k_str();
            if ( str!=NULL  &&  strlen(str)<STAFILE_LEN ) 
            {
               STAFILE *tmp;
               tmp = (STAFILE *)realloc( Gparm->StaFile, (Gparm->nStaFile+1)*sizeof(STAFILE) );
               if( tmp == NULL ) 
               {
                  logit( "e", "pick_ew: Error reallocing Gparm->StaFile. Exiting.\n" );
                  return -1;
               }
               Gparm->StaFile = tmp;
               strcpy( Gparm->StaFile[Gparm->nStaFile].name, str );
               Gparm->StaFile[Gparm->nStaFile].nsta = 0;
               Gparm->nStaFile++;
            } else {
               logit( "e", "pick_ew: Invalid StaFile name <%s>; must be 1-%d chars. " 
                      "Exiting.\n", str, STAFILE_LEN );
               return -1;
            }
            init[0] = 1;
         }

         else if ( k_its( "InRing" ) )
         {
            if ( (str = k_str()) != NULL )
            {
               if( (Gparm->InKey = GetKey(str)) == -1 )
               {
                  logit( "e", "pick_ew: Invalid InRing name <%s>. Exiting.\n", str );
                  return -1;
               }
            }
            init[1] = 1;
         }

         else if ( k_its( "OutRing" ) )
         {
            if ( (str = k_str()) != NULL )
            {
               if ( (Gparm->OutKey = GetKey(str)) == -1 )
               {
                  logit( "e", "pick_ew: Invalid OutRing name <%s>. Exiting.\n", str );
                  return -1;
               }
            }
            init[2] = 1;
         }

         else if ( k_its( "HeartbeatInt" ) )
         {
            Gparm->HeartbeatInt = k_int();
            init[3] = 1;
         }

         else if ( k_its( "RestartLength" ) )
         {
            Gparm->RestartLength = k_int();
            init[4] = 1;
         }

         else if ( k_its( "MaxGap" ) )
         {
            Gparm->MaxGap = k_int();
            init[5] = 1;
         }

         else if ( k_its( "Debug" ) )
         {
            Gparm->Debug = k_int();
            init[6] = 1;
         }

         else if ( k_its( "MyModId" ) )
         {
            if ( (str = k_str()) != NULL )
            {
               if ( GetModId(str, &Gparm->MyModId) == -1 )
               {
                  logit( "e", "pick_ew: Invalid MyModId <%s>.\n", str );
                  return -1;
               }
            }
            init[7] = 1;
         }
 /*opt*/ else if ( k_its( "NoCoda" ) )
         {
            Gparm->NoCoda = k_int();
         }
 /*opt*/ else if ( k_its( "NoCodaHorizontal" ) )
         {
            Gparm->NoCodaHorizontal = k_int();
         }
 /*opt*/ else if ( k_its( "PickIndexDir" ) )
         {
            Gparm->PickIndexDir = strdup(k_str());

#if defined(_WINNT)
            if(_access(Gparm->PickIndexDir, 00) == -1) {
               //POSIX-compliant 'mkdir' is deprecated on Windows:
               if(_mkdir(Gparm->PickIndexDir) == -1) {
                  //Give up and go home:
                  free(Gparm->PickIndexDir);
                  Gparm->PickIndexDir = NULL;
               }
	    }
#else
            if(access(Gparm->PickIndexDir, F_OK) == -1) {  
               if(mkdir(Gparm->PickIndexDir, 0700) == -1) {
                  //Give up and go home:
                  free(Gparm->PickIndexDir);
                  Gparm->PickIndexDir = NULL;
               }
	    }
#endif
         }

 
 /*opt*/ else if ( k_its( "GetLogo" ) )
         {
            MSG_LOGO *tlogo = NULL;
            int       nlogo = Gparm->nGetLogo;
            tlogo = (MSG_LOGO *)realloc( Gparm->GetLogo, (nlogo+1)*sizeof(MSG_LOGO) );
            if( tlogo == NULL )
            {
               logit( "e", "pick_ew: GetLogo: error reallocing %zu bytes.\n",
                      (nlogo+1)*sizeof(MSG_LOGO) );
               return -1;
            }
            Gparm->GetLogo = tlogo;
            
            if( (str=k_str()) != NULL)       /* read instid */
            {
               if( GetInst( str, &(Gparm->GetLogo[nlogo].instid) ) != 0 ) 
               {
                  logit( "e", "pick_ew: Invalid installation name <%s>"
                         " in <GetLogo> cmd!\n", str );
                  return -1;
               }
               if( (str=k_str()) != NULL )    /* read module id */
               {
                  if( GetModId( str, &(Gparm->GetLogo[nlogo].mod) ) != 0 ) 
                  {
                     logit( "e", "pick_ew: Invalid module name <%s>"
                            " in <GetLogo> cmd!\n", str );
                     return -1;
                  }
                  if( (str=k_str()) != NULL) /* read message type */
                  {
                     if( strcmp(str,"TYPE_TRACEBUF") !=0 &&
                         strcmp(str,"TYPE_TRACEBUF2")!=0    )
                     {
                        logit( "e","pick_ew: Invalid message type <%s> in <GetLogo>"
                               " cmd; must be TYPE_TRACEBUF or TYPE_TRACEBUF2.\n",
                                str );
                        return -1;
                     }
                     if( GetType( str, &(Gparm->GetLogo[nlogo].type) ) != 0 ) {
                        logit( "e", "pick_ew: Invalid message type <%s>"
                               " in <GetLogo> cmd!\n", str );
                        return -1;
                     }
                     Gparm->nGetLogo++;
                  } /* end if msgtype */
               } /* end if modid */  
            } /* end if instid */  
         } /* end GetLogo cmd */

/* An unknown parameter was encountered
   ************************************/
         else
         {
            logit( "e", "pick_ew: <%s> unknown parameter in <%s>\n",
                    com, config_file );
            continue;
         }

/* See if there were any errors processing the command
   ***************************************************/
         if ( k_err() )
         {
            logit( "e", "pick_ew: Bad <%s> command in <%s>.\n", com,
                     config_file );
            return -1;
         }
      }
      nfiles = k_close();
   }

/* After all files are closed, check flags for missed commands
   ***********************************************************/
   nmiss = 0;
   for ( i = 0; i < ncommand; i++ )
      if ( !init[i] )
         nmiss++;

   if ( nmiss > 0 )
   {
      logit( "e", "pick_ew: ERROR, no " );
      if ( !init[0] ) logit( "e", "<StaFile> " );
      if ( !init[1] ) logit( "e", "<InRing> " );
      if ( !init[2] ) logit( "e", "<OutRing> " );
      if ( !init[3] ) logit( "e", "<HeartbeatInt> " );
      if ( !init[4] ) logit( "e", "<RestartLength> " );
      if ( !init[5] ) logit( "e", "<MaxGap> " );
      if ( !init[6] ) logit( "e", "<Debug> " );
      if ( !init[7] ) logit( "e", "<MyModId> " );
      logit( "e", "command(s) in <%s>. Exiting.\n", config_file );
      return -1;
   }
   return 0;
}


 /***********************************************************************
  *                              LogConfig()                            *
  *                                                                     *
  *                   Log the configuration parameters                  *
  ***********************************************************************/

void LogConfig( GPARM *Gparm )
{
   int i;

   logit( "", "\n" );
   logit( "", "nStaFile:        %6d\n",   Gparm->nStaFile );
   for( i=0; i<Gparm->nStaFile; i++ ) {
      logit( "", "StaFile[%d]:    %s\n",  i, Gparm->StaFile[i].name );
   }
   logit( "", "InKey:           %6ld\n",  Gparm->InKey );
   logit( "", "OutKey:          %6ld\n",  Gparm->OutKey );
   logit( "", "HeartbeatInt:    %6d\n",   Gparm->HeartbeatInt );
   logit( "", "RestartLength:   %6d\n",   Gparm->RestartLength );
   logit( "", "MaxGap:          %6d\n",   Gparm->MaxGap );
   logit( "", "Debug:           %6d\n",   Gparm->Debug );
   logit( "", "MyModId:         %6u\n",   Gparm->MyModId );
   logit( "", "nGetLogo:        %6d\n",   Gparm->nGetLogo );
   for( i=0; i<Gparm->nGetLogo; i++ ) {
      logit( "", "GetLogo[%d]:   i%u m%u t%u\n", i,
            Gparm->GetLogo[i].instid, Gparm->GetLogo[i].mod, 
            Gparm->GetLogo[i].type );
   }

   return;
}
