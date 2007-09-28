#include "RecoveryInterface.h"

void recovery_progress_callback()
{
	fprintf(stderr, "Recovery progress callback...\n");
}

int recovery_restore(string *args, struct shell_state *sh)
{
	
	mach_error_t ret;
	
	if (args[1] == "")
	{
		ifNotQuiet cout << "args[1] must be RestoreBundlePath" << endl;
		return SHELL_CONTINUE;
	}
	
	D("Building CFMutableDictionary");
	CFMutableDictionaryRef opts;
	D("Getting AMRestoreCreateDefaultOptions");
	opts = AMRestoreCreateDefaultOptions(kCFAllocatorDefault);
	CFDictionarySetValue(opts, CFSTR("RestoreBundlePath"), args[1].c_str() );
        
	//    describe255(opts);
        
	ret = AMRestorePerformRecoveryModeRestore(	sh->recovery_dev, opts,
							(void *)recovery_progress_callback, NULL );
	
	ifVerbose cout 	<< "AMRestorePerformRecoveryModeRestore: " << ret << endl;
	
	return SHELL_CONTINUE;
}

int recovery_grestore(string *args, struct shell_state *sh)
{
	int ret;
	
	if (args[1] == "")
	{
		ifNotQuiet cout << "args[1] must be RestoreBundlePath" << endl;
		return SHELL_CONTINUE;
	}
	
	// check for trailing /
	if ( args[1].at(args[1].length() - 1) != '/' )
		args[1] = args[1] + '/';
	
	//string temp = args[1] + "009-7698-4.dmg";
	string temp = args[1] + "022-3604-4.dmg";
	
	ifNotQuiet cout << "Entering G-Restore ... " << endl;
	
	// Send ramdisk to the phone
	ifVerbose cout << "Sending ramdisk '"<< temp << "'" << endl;
	ret = sendFileToDevice(sh->recovery_dev, CFStringCreateWithCString(NULL, temp.c_str(), kCFStringEncodingASCII));
	ifVerbose cout << "sendFileToDevice: " << ret << endl;
	if (ret != 0) {
		ifNotQuiet cout << "Send ramdisk failed.  Aborting." << endl;
		return SHELL_CONTINUE;
	}
	
	// load ramdisk on the phone
	ifVerbose cout << "Loading ramdisk." << endl;
	ret = sendCommandToDevice(sh->recovery_dev, CFSTR("ramdisk"));
	ifVerbose cout << "sendCommandToDevice 'ramdisk': " << ret << endl;
	if (ret != 0) {
		ifNotQuiet cout << "Load ramdisk failed.  Aborting." << endl;
		return SHELL_CONTINUE;
	}
	
	//temp = args[1]+ "kernelcache.restore.release.s5l8900xrb";
	temp = args[1]+ "kernelcache.release.s5l8900xrb";
	
	// Send the kernelcache
	ifVerbose cout << "Sending kernelcache '"<< temp << "'" << endl;
	ret = sendFileToDevice(sh->recovery_dev, CFStringCreateWithCString(NULL, temp.c_str(), kCFStringEncodingASCII));
	ifVerbose cout << "sendFileToDevice: " << ret << endl;
	if (ret != 0) {
		ifNotQuiet cout << "Send kernelcache failed.  Aborting." << endl;
		return SHELL_CONTINUE;
	}
	
	//check for custom bootargs
	if( args[2] != "" )
		D("grestore does not yet except custom bootargs.");
	
	ifVerbose cout << "Setting bootargs: setenv boot-args rd=md0 -v" << endl;
	ret = sendCommandToDevice(sh->recovery_dev, CFSTR("setenv boot-args rd=md0 -v"));
	ifVerbose cout << "sendCommandToDevice: " << ret << endl;
	if (ret != 0) {
		ifNotQuiet cout << "Setevn command failed.  Aborting." << endl;
		return SHELL_CONTINUE;
	}
	
	ifVerbose cout << "Loading kernelcache (command: bootx)" << endl;
	ret = sendCommandToDevice(sh->recovery_dev, CFSTR("bootx"));
	ifVerbose cout << "sendCommandToDevice: " << ret << endl;	
	if (ret != 0) {
		ifNotQuiet cout << "failed (" << ret << ") Aborting." << endl;
		return SHELL_CONTINUE;
	}
	
	ifNotQuiet cout << "G-Restore Completed.  \nExit iPHUC or reconnect in order to continue in restore mode." << endl;
	
	return SHELL_CONTINUE;
}

int recovery_filecopytophone(string *args, struct shell_state *sh)
{
	mach_error_t retval = sendFileToDevice(	sh->recovery_dev,
						CFStringCreateWithCString(NULL, args[1].c_str(), kCFStringEncodingASCII) );
	
	ifNotQuiet cout	<< "filecopytophone: " << retval << endl;
	//cout << "filecopytophone: " << filecopytophone(sh->recovery_dev, cline.c_str(), 0x09000000) << " bytes copied" << endl;
	
	return SHELL_CONTINUE;
}

static t_AMRUSBInterfaceReadPipe _AMRUSBInterfaceReadPipe = (t_AMRUSBInterfaceReadPipe)0x3c3a437c;

int recovery_cmd(string *args, struct shell_state *sh)
{
	if ( args[1] == "" )
	{
		ifNotQuiet cout << "Please provide a command to send to the phone." << endl;
		return SHELL_CONTINUE;
	}
	
	
	char commandLine[1024] = "";
	for(int i = 1; i < MAX_ARGUMENTS; i++) {
		if(args[i] != "") {
			strlcat(commandLine, args[i].c_str(), sizeof(commandLine));
			strlcat(commandLine, " ", sizeof(commandLine));
		}
	}
	mach_error_t retval = sendCommandToDevice(	sh->recovery_dev,
						CFStringCreateWithCString(NULL, commandLine, kCFStringEncodingASCII) );

	cout	<< "cmd: " << commandLine << ": " << retval << endl; 

//	CFMutableDictionaryRef dict = CFDictionaryCreateMutable( kCFAllocatorDefault, 0,
//			                                &kCFTypeDictionaryKeyCallBacks,
//											                                &kCFTypeDictionaryValueCallBacks);
	
//	ifVerbose cout << "waiting for a response..." << endl;
//	int result = waitForResponse(sh->recovery_dev, dict);
//	ifVerbose cout << "result = " << result << endl;
//	CFShow(dict);

	unsigned char buffer[1024];
	unsigned int length = sizeof(buffer);
	retval = _AMRUSBInterfaceReadPipe(sh->recovery_dev->readwrite_pipe, sh->recovery_dev->read_pipe, buffer, &length);

	cout << "retval = " + retval << endl;

	printf("user_info = 0x%8.8x\n", sh->recovery_dev->user_info);
	printf("readwrite_pipe = 0x%8.8x\n", sh->recovery_dev->readwrite_pipe);
	printf("read_pipe = 0x%8.8x\n", sh->recovery_dev->read_pipe);

	return SHELL_CONTINUE;
}


int recovery_serial(string *args, struct shell_state *sh)
{
	ifNotQuiet cout << "Restarting in serial mode." << endl;
        
	D("bgcolor 255 255 0");
	sendCommandToDevice(sh->recovery_dev, CFSTR("bgcolor 255 255 0"));
	D("setenv debug-uarts 1");
	sendCommandToDevice(sh->recovery_dev, CFSTR("setenv debug-uarts 1"));
	D("saveenv");
	sendCommandToDevice(sh->recovery_dev, CFSTR("saveenv"));
	D("reboot");
	sendCommandToDevice(sh->recovery_dev, CFSTR("reboot"));
	
	//im ASSUMING we dont want to kill the app here, but that we want to
	// at least exit recovery mode (?)
	
	ifNotQuiet cout << "Please restart iPHUC or reconnect in order to issue serial commands." << endl;
	
	return SHELL_CONTINUE;
}

int recovery_exit(string *args, struct shell_state *sh)
{
	D("exiting iphuc");
	exit(0);
}

int recovery_disconnect(string *args, struct shell_state *sh)
{
	return SHELL_WAIT;
}
