/*                                                                                                                                                                                                             \
 * Filename      program5.cc
 * Date          4/14/20
 * Author        Payton Harmon
 * Email         pnh170000@utdallas.edu
 * Course        CS 3377.502 Spring 2020
 * Version       1.0  (or correct version)
 * Copyright 2020, All Rights Reserved
 *
 * Description
 *
 *    Creates a process that notifies the output of any modifications in the directory listed in the config.
 *    Can be ran as a daemon outputting to the log file our as a regular process outputting to the stdout.    
 *
 */

#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include "program5.h"




// Gobal Vars
fstream logFile;
bool sighup;

string runCmd(string arg);

void signalHandler( int signum )
{
  cout << "Interrupt signal (" << signum << ") received." << endl; 
  switch(signum)
  {
    case SIGINT:
    case SIGTERM:
    { logFile.close();
      remove("cs3377dirmond.pid");
    } exit(0);

    case SIGHUP:
      sighup = true;
      break;

    default:
      return;
  }

}


int main(int argc, char* argv[])
{


  // Initialize the map;
  map<int,string> argMap = initMap(argc,argv);

  // Map each key value to a variable.
  string configFile = argMap.find(CONFIG)->second;
  string daemon = argMap.find(DAEMON)->second;
  
  // Initialize the config map.
  fstream config(configFile.c_str());
  map<int,string> configMap;
  if (!config)
  {
    // Can't open file
    cerr << "Unable to open the config file" << endl;
    return 1;
  }
  else
  {
    configMap = parseConfig(configFile);
  }
  config.close();

  // Map each config to string
  string verbose = configMap.find(VERBOSE)->second;
  string logFileName = configMap.find(LOGFILE)->second;
  string password = configMap.find(PASSWORD)->second;
  string numVersion = configMap.find(NUMVERSION)->second;
  string watchDir = configMap.find(WATCHDIR)->second;

  //Create files
  fstream pidFile;
  fstream logFile;

  // Running in daemon mode.
  if(daemon == "1")
  {
    pid_t forkValue;

    forkValue = fork();

    if (forkValue == -1) // no child created
    {
      cerr << "Error: No child was created in the fork." << endl;
      return 1;
    }
    else if (forkValue == 0) // this is the child
    {
      //open log file
      logFile.open(logFileName.c_str(), ios::out);

      // Redirect cout to got to the log file
      cout.rdbuf(logFile.rdbuf()); 
    }
    else // this is the parent
    {
      // kill the parent
      return 0;
    }
 }

 // Create and close pid file;
  pid_t pid = getpid();
  pidFile.open("cs3377dirmond.pid", ios::out);
  pidFile << pid << endl;
  pidFile.close();

  // Make the .versions file
  string cmd = "mkdir " + watchDir + "/.versions";
  system(cmd.c_str());

  //sighup = false;
  // here is where both modes will operate regardless of output method
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
  signal(SIGHUP, signalHandler);
  siginterrupt(SIGHUP, 1);

  // Setup inotify
  size_t bufsiz = sizeof(struct inotify_event) + PATH_MAX + 1;
  struct inotify_event *event = (struct inotify_event *) malloc(bufsiz);
  char buffer[bufsiz];


  int fd = inotify_init();
  int wd = inotify_add_watch(fd, watchDir.c_str(), IN_MODIFY);

  

  while(true)
  {
    int i = 0;
    int length = read(fd, buffer, bufsiz);
    if(length == -1 && errno == EINTR)
    {
      if(sighup == true)
      {
	sighup = false;
	cout << "Updating configMap" << endl;
	configMap = parseConfig(configFile);
	verbose = configMap.find(VERBOSE)->second;
	logFileName = configMap.find(LOGFILE)->second;
	if(password != configMap.find(PASSWORD)->second)
	{
	  cout << "Unable to change password setting while running" << endl;
	}
	numVersion = configMap.find(NUMVERSION)->second;
	if(watchDir != configMap.find(WATCHDIR)->second)
	{
	  cout << "Unable to change watchDir while running" << endl;
	}  
      }
    }
    else
    {
      // react to the read;
      while ( i < length )
	{
	  event = ( struct inotify_event * ) &buffer[ i ];
	  string date = runCmd("date +%y.%m.%d-%H:%M:%S");
	  string name = (string) event->name + "." + date;
	  if ( event->len )
	    {
	      if ( event->mask & IN_MODIFY )
		{
		  if ( event->mask & IN_ISDIR )
		    {
		      cout << "The directory " << event->name << " was modified." << endl;
		    }
		  else
		    {
		      cout << "The file " << event->name << " was modified." << endl;
		    }
		  if(verbose == "true")
		    cout << "Copying to .versions directory as " << name << endl;
		  cmd = "cp " + watchDir + "/" + (string) event->name + " " + watchDir + "/.versions/"+name;
		  system(cmd.c_str());
		}
	    }
	  i += sizeof( struct inotify_event) + event->len;
	}
    }

  }


  return 0;
}


// Run a command using popen() and return the result in a std::string
string runCmd (string arg)
{
  FILE *output_from_command;
  char tmpbuffer[1024];
  char *line_p;

  // Get the output of the call to gawk.
  output_from_command = popen(arg.c_str(), "r");
  
  // Convert the pipe into cstr
  line_p = fgets(tmpbuffer, 1024, output_from_command);

  // convert c string to std::string;
  string str(line_p);

  // Close the pipe
  pclose(output_from_command);

  // Return output
  return str;
}
