#include"BLK_API.cpp"

int main()
{
	OleBLK myInstance;
	myInstance.ConnectBLK("10.10.1.1");
	BlkConfig myExample = myInstance.GetDeviveInfo();
	common::print(myExample.deviceInfo);
	common::print(myExample.deviceStatus);
	myInstance.DoScan();
	system("pause");
	return 0;
}