#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include "NetworkController.hpp"
#include "CmdLineInterface.hpp"
#include "AppConfig.hpp"
#include "GUIManager.hpp"
#include "VideoDevice.hpp"
#include "LDetector.hpp"
#include "LProcessor.hpp"


int main(int argc, char* argv[])
{
    // get command line interface config options
    CmdLineInterface interface(argc, argv);
    AppConfig config = interface.getConfig();

    GUIManager gui;
    VideoDevice camera;
    LProcessor processor;
    NetworkController networkController;

    //init camera
    if(config.getIsDevice())
    {
        camera.startCapture(config.getDeviceID());
        if(config.getIsDebug())
            std::cout << "Camera ready!\n";
    }

    //init networking
    if(config.getIsNetworking())
        networkController.startServer();

    if(!config.getIsHeadless())
        gui.init();

    //continuous server loop
    do
    {
        if(config.getIsNetworking())
            networkController.waitForPing();

        LDetector detector;

        cv::Mat image;
        if(config.getIsFile())
        {
            image = cv::imread(config.getFileName());
        }
        else
        {
            image = camera.getImage(config.getIsDebug());
        }

        detector.elLoad(image);
        detector.elSplit();
        detector.elThresh();
        detector.elContours();
        detector.elFilter();

	bool foundL = true;
	if (detector.getLs().size() >= 2)
		detector.largest2();
	else
		foundL = false;
	if (detector.getLs().size() < 2)
		foundL = false;
	if (foundL)
	{
            if(config.getIsDebug())
		    std::cout << "Found " << detector.getLs().size() << " L's!" << std::endl;
            processor.determineL(detector.getLs());
            processor.determineAzimuth();
            processor.determineDistance();
            double azimuth = processor.getAzimuth();
            double distance = processor.getDistance();

            if(!config.getIsHeadless())
            {
                gui.setImage(detector.show());
                gui.setImageText("Found " + boost::lexical_cast<std::string>(detector.getLs().size()) + " L's");
                gui.show(config.getIsFile());
            }

            if(config.getIsDebug())
            {
                processor.outputData();
                std::cout << "Final distance: " << processor.getDistance() << std::endl;
            }

            if(config.getIsNetworking())
            {
                networkController.sendMessage(boost::lexical_cast<std::string> ("true") + std::string(";") 
                    + boost::lexical_cast<std::string> (distance) + std::string(";") 
                    + boost::lexical_cast<std::string> (azimuth));
            }

        }
        else 
        {
            if(config.getIsDebug() && config.getIsFile())
                std::cout << "Found less than 2 L's :(\n";
            if(config.getIsNetworking())
                networkController.sendMessage(boost::lexical_cast<std::string> ("false") + std::string(";"));
        }

        
    }
    while(config.getIsDevice());

    return 0;
}

