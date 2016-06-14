#include "DynamixelConsole.h"

const DynamixelCommand DynamixelConsole::sCommand[] =
	{{"ping", &DynamixelConsole::ping},
	{"read", &DynamixelConsole::read},
	{"write", &DynamixelConsole::write}};

DynamixelConsole::DynamixelConsole(DynamixelInterface &aInterface, Stream &aConsole):
	mInterface(aInterface), mConsole(aConsole)
{
	mLinePtr=&(mLineBuffer[0]);
	mLineBuffer[sLineBufferSize]=0;
}

void DynamixelConsole::loop()
{
	//empty input buffer
	while(mConsole.available())
		mConsole.read();
	
	//write new command prompt
	mConsole.write(">");
	
	// read one command line
	char c;
	while((c=mConsole.read())!='\n' && c!='\r')
	{
		if(c>=32 && c<=126 && (mLinePtr-&(mLineBuffer[0]))<sLineBufferSize)
		{
			mConsole.write(c);
			*mLinePtr=c;
			++mLinePtr;
		}
		else if(c==8 && mLinePtr>(&(mLineBuffer[0])))
		{
			mConsole.write(c);
			--mLinePtr;
		}
	}
	//new line
	mConsole.write("\n\r");
	// run command
	run();
	// reset buffer
	mLinePtr=&(mLineBuffer[0]);
}

void DynamixelConsole::run()
{
	char *argv[16];
	int argc=parseCmd(argv);
	
	const int commandNumber=sizeof(sCommand)/sizeof(DynamixelCommand);
	for(int i=0; i<commandNumber; ++i)
	{
		if(strcmp(argv[0],sCommand[i].mName)==0)
		{
			DynamixelStatus status=(this->*(sCommand[i].mCallback))(argc, argv);
			printStatus(status);
			break;
		}
	}
}

int DynamixelConsole::parseCmd(char **argv)
{
	int argc=0;
	char *ptr=&mLineBuffer[0];
	while(argc<15)
	{
		while(*ptr==' ' && ptr<mLinePtr)
		{
			++ptr;
		}
		if(ptr>=mLinePtr)
		{
			break;
		}
		argv[argc]=ptr;
		while(*ptr!=' ' && ptr<mLinePtr)
		{
			++ptr;
		}
		*ptr='\0';
		++ptr;
		++argc;
	}
	argv[argc]=0;
	return argc;
}


void DynamixelConsole::printStatus(DynamixelStatus aStatus)
{
	if(aStatus==DYN_STATUS_INTERNAL_ERROR)
	{
		mConsole.print("Invalid command parameters\n\r");
		return;
	}
	mConsole.print("Status : ");
	if(aStatus==DYN_STATUS_OK)
	{
		mConsole.print("ok");
	}
	else if(aStatus&DYN_STATUS_COM_ERROR)
	{
		mConsole.print("communication error");
		if(aStatus&DYN_STATUS_TIMEOUT)
		{
			mConsole.print(", timeout");
		}
		else if(aStatus&DYN_STATUS_CHECKSUM_ERROR)
		{
			mConsole.print(", invalid response checksum");
		}
	}
	else
	{
		mConsole.print("communication ok");
		
		if(aStatus&DYN_STATUS_INPUT_VOLTAGE_ERROR)
		{
			mConsole.print(", invalid input voltage");
		}
		if(aStatus&DYN_STATUS_ANGLE_LIMIT_ERROR)
		{
			mConsole.print(", angle limit error");
		}
		if(aStatus&DYN_STATUS_OVERHEATING_ERROR)
		{
			mConsole.print(", overheating");
		}
		if(aStatus&DYN_STATUS_RANGE_ERROR)
		{
			mConsole.print(", out of range value");
		}
		if(aStatus&DYN_STATUS_CHECKSUM_ERROR)
		{
			mConsole.print(", invalid command checksum");
		}
		if(aStatus&DYN_STATUS_OVERLOAD_ERROR)
		{
			mConsole.print(", overload");
		}
		if(aStatus&DYN_STATUS_INSTRUCTION_ERROR)
		{
			mConsole.print(", invalid instruction");
		}
	}
	mConsole.print("\n\r");
}

void DynamixelConsole::printData(const uint8_t *data, uint8_t lenght)
{
	for(uint8_t i=0; i<lenght; ++i)
	{
		mConsole.print(data[i]);
		mConsole.print(" ");
	}
	mConsole.print("\n\r");
}

DynamixelStatus DynamixelConsole::ping(int argc, char **argv)
{
	int id=0;
	if(argc<2)
	{
		mConsole.print("Usage : ping <id>\n\r");
		return DYN_STATUS_INTERNAL_ERROR;
	}
	id=atoi(argv[1]);
	if(id<1 || id>254)
	{
		return DYN_STATUS_INTERNAL_ERROR;
	}
	return mInterface.ping(id);
}

DynamixelStatus DynamixelConsole::read(int argc, char **argv)
{
	int id=0, addr=0, lenght=1;
	if(argc<3)
	{
		mConsole.print("Usage : read <id> <address> <length=1>\n\r");
		return DYN_STATUS_INTERNAL_ERROR;
	}
	id=atoi(argv[1]);
	if(id<1 || id>254)
	{
		return DYN_STATUS_INTERNAL_ERROR;
	}
	addr=atoi(argv[2]);
	if(argc>3)
	{
		lenght=atoi(argv[3]);
	}
	if(lenght>255)
	{
		return DYN_STATUS_INTERNAL_ERROR;
	}
	uint8_t *ptr=new uint8_t[max(2,lenght)];
	DynamixelStatus result=mInterface.read(id, addr, lenght, ptr);
	printData(ptr,lenght); 
	delete ptr;
	return result;
}

DynamixelStatus DynamixelConsole::write(int argc, char **argv)
{
	int id=0, addr=0, lenght=0;
	if(argc<4)
	{
		mConsole.print("Usage : write <id> <address> <data_1> ... <data_N>\n\r");
		return DYN_STATUS_INTERNAL_ERROR;
	}
	id=atoi(argv[1]);
	if(id<1 || id>254)
	{
		return DYN_STATUS_INTERNAL_ERROR;
	}
	addr=atoi(argv[2]);
	lenght=argc-3;
	if(lenght>255)
        {
                return DYN_STATUS_INTERNAL_ERROR;
        }
	
	uint8_t *ptr=new uint8_t[lenght+1];
	for(uint8_t i=0; i<lenght; ++i)
	{
		ptr[i+1]=atoi(argv[i+3]);
	} 
	DynamixelStatus result=mInterface.write(id, addr, lenght, ptr+1);
	delete ptr;
	return result;
}

