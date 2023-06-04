#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ESP32Time.h>


/////////////////////////////////////////////////////////////////
#if 1/* Setup wifi,GPIO and firebase */

#define WIFI_SSID "fill out here"
#define WIFI_PASSWORD "fill out here"

#define OnLed HIGH
#define OffLed LOW

#define FIREBASE_HOST "fill out here"
#define FIREBASE_AUTH "fill out here"


#endif
/////////////////////////////////////////////////////////////////
#if 1/* Global variable */
	  FirebaseData database;
	  FirebaseData toggleWifi;

  // Real time clock
	  ESP32Time rtc;

	// Data recieve from firebase
	  unsigned char dataSend[6];
	  
	/* Data firebase */
	// Data download
	  unsigned char Led = 0;
	  unsigned char alarmLed = 0;
	  unsigned char startHour = 0;
	  unsigned char startMinute = 0;
	  unsigned char endHour = 0;
	  unsigned char endMinute = 0;
	  unsigned char wifi = 0;
	  
	  
	/* Limited Time */
	  unsigned char sHour = 0 ;
	  unsigned char sMinute = 0;
	  unsigned char eHour = 0;
	  unsigned char eMinute = 0;

  /* Time for alarm */
	  struct timeAlarm {
		  unsigned char Hours;
		  unsigned char Minutes;
	  };
	  struct timeAlarm timeNow;
	 
	  
	// Data upload 
	  unsigned char statusLed = 0;
	  unsigned char statusAlarm = 0;
	  unsigned char statusWifi = 0;
	  
	/* Var global flag */
	  unsigned char glReadFireBaseFlag = 0;
	  unsigned char glUpLoadDataBase = 0;
	  unsigned char glResetTimerFlag = 0;
	  unsigned char glDisconnectWifiFlag = 0;
	  unsigned char glLimitedTimeFlag = 0;
	  

	/* Variable for millis */
	  unsigned long previousMillis = 0;
	  unsigned long currentMillis  = 0;
	  const long interval = 2000;  //2 second
	  
	  
	/* Time Interval for wait reconnecting wifi */
	  unsigned long currentConnectWifi = 0;
	  unsigned long previousConnectWifi = 0;
	  const long connectWifiInterval = 5000; // 5 second 
	  
	
	/* Limit Time Pump */
	  unsigned char timePlay = 1;

	  
	  
  
#endif
/////////////////////////////////////////////////////////////////
#if 1/* Implement function */
		//void IRAM_ATTR onTimer();   // interupt timer for loop 10ms
		void setup();
		
	/* Task connecting wifi core 0*/
		void connectWifi(void *pvParameters);
		
		void loop();
		void operate();


	/* write and read Data firebase */
		unsigned char readDataFireBase();
		void writeDataFirebase();
	  
	/* Set power Pump */
		void setPowerLed(unsigned char mode);
		
	/* Check data */
		void queryData();
		
	/* Alarm Pump */
	  	void setTime(unsigned char hour, unsigned char minute);
		void getTime();
		unsigned char alarm(unsigned char hour, unsigned char minute);
	
	/* Limited time for Pumping */
		void setLimitedTime();
		
	/* Clear database */ 
	  	void clearData();
		
	/* check connect esp32 to wifi */  
		unsigned char Wifi();
		unsigned char toggle(unsigned char a);


#endif
/////////////////////////////////////////////////////////////////
  

/////////////////////////////////////////////////////////////////
void setup() 
{
	  // put your setup code here, to run once:
	  // set up baurate UART
	  Serial.begin(9600);


/* Config Wifi */
#if 1	  
	  /* connecting wifi */
	  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	  Serial.print("Connecting to Wifi");
	  while(WiFi.status() != WL_CONNECTED){
		  Serial.print(".");
		  delay(300);
	  }
	  Serial.println();
	  Serial.print("Connected with IP: ");
	  Serial.println(WiFi.localIP());
	  Serial.println();

#endif

	

#if 0 /* Smart Config Wifi */	    
	  WiFi.mode(WIFI_AP_STA);
	  /* start SmartConfig */
	  WiFi.beginSmartConfig();
	  /* Wait for SmartConfig packet from mobile */
	  Serial.println("Waiting for SmartConfig.");
      while (!WiFi.smartConfigDone()) {
         delay(500);
         Serial.print(".");
      }
      Serial.println("");
      Serial.println("SmartConfig done.");
 
      /* Wait for WiFi to connect to AP */
      Serial.println("Waiting for WiFi");
      while (WiFi.status() != WL_CONNECTED) {
         delay(500);
         Serial.print(".");
      }
      Serial.println("WiFi Connected.");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
#endif
	

  /* connecting Firebase */
	Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);
	  
	Firebase.setReadTimeout(database,1000*60);
	Firebase.setwriteSizeLimit(database,"tiny");
	  
	Firebase.setReadTimeout(toggleWifi,1000*60);
	Firebase.setwriteSizeLimit(toggleWifi,"tiny");

	//disableCore0WDT();
	  
	  /* Config GPIO */
	  // define Pump  = GPIO_NUM_23  control Relay
	  pinMode(GPIO_NUM_23, OUTPUT);
	  
}
/////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////


 /* Loop main */ 
void loop()
{

  	operate();
	
	/* Reconnecting Wifi */
	if(WiFi.status() != WL_CONNECTED){
		WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
			
		/* Loop 5s wait reconnecting Wifi */
		currentConnectWifi = millis();
		previousConnectWifi = currentConnectWifi;
		while((currentConnectWifi - previousConnectWifi < connectWifiInterval)&&(WiFi.status() != WL_CONNECTED)){
			currentConnectWifi = millis();
		}	
	}
	
	
	/* Loop 2s */
	currentMillis  = millis();
	if(currentMillis - previousMillis >= interval && Wifi() == 1 ){
		previousMillis = currentMillis;
		
		// read data from Firebase
		readDataFireBase();
		glReadFireBaseFlag = 1;        //glReadFireBaseFlag = 1 just here
		
		/* query Database */
		if(glReadFireBaseFlag == 1){
			/* query Data */
			queryData();
			
			glUpLoadDataBase = 1;        //glUpLoadDataBase = 1 just here
			
			// Delete ReadFirebase Flag
			glReadFireBaseFlag = 0;
		}	
	}
	
}


void operate()
{
	/* Alarm Pump */
    if(statusAlarm){
		
		if(glLimitedTimeFlag == 1){
			statusAlarm = alarm(eHour,eMinute);
		}
		else{
			statusAlarm = alarm(endHour,endMinute);
		}
		
		if(statusAlarm == 0){
			// turn Off Led
			setPowerLed(OffLed);
			statusLed = 0;
			glLimitedTimeFlag = 0;
			
			//Serial.println("finnish alarm");
	
			// Second Update 
			glResetTimerFlag = 1;
			glUpLoadDataBase = 1;
		}
		
		// continute alarm-------->
    }
	
	// Update data Firebase
	if(glUpLoadDataBase == 1){
		
		/* Toggle logic wifi */
		statusWifi = toggle(statusWifi);

		/* Update data -> Firebase */ 
		writeDataFirebase();
		
		// clear flag 
		glUpLoadDataBase = 0;
	}
	
	
}
////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////
/* write and read Data firebase */
unsigned char readDataFireBase()
{
	  //unsigned char flag;
	 

	  /* Download database */
	  
	  if(Firebase.getInt(database,"/dataApptoMCU/Led") != true){
		  goto END;
	  }
	  Led = database.to<int>();
	  
	  if(Firebase.getInt(database,"/dataApptoMCU/alarmLed") != true){
		  goto END;
	  }
	  alarmLed = database.to<int>();
	  

	  if(alarmLed == 1 && statusAlarm == 0){
		//Serial.print("read alarm");
		delay(1000);
		if(Firebase.getInt(database,"/dataApptoMCU/startHour") != true){
		  goto END;
		}
		startHour = database.to<int>();
		
		if(Firebase.getInt(database,"/dataApptoMCU/startMinute") != true){
			goto END;
		}
		startMinute = database.to<int>();
		
		if(Firebase.getInt(database,"/dataApptoMCU/endHour") != true){
			goto END;
		}
		endHour = database.to<int>();
		
		if(Firebase.getInt(database,"/dataApptoMCU/endMinute") != true){
			goto END;
		}
		endMinute = database.to<int>();
	  }
	  
	  return 1;
  
END: return 0;    
}

void writeDataFirebase()
{
	
	/* Upload data firebase */ 
	Firebase.setInt(database, "/dataMCUtoApp/statusLed",statusLed);

  /* Update Status Wifi */
	Firebase.setInt(database, "dataMCUtoApp/statusWifi",statusWifi);
	
	
	/* Reset Timer ALarm Pump*/
	if(glResetTimerFlag == 1 ){
	  	Firebase.setInt(database, "/dataApptoMCU/Led",0);
		Firebase.setInt(database, "/dataApptoMCU/alarmLed",0);
		Firebase.setInt(database, "/dataApptoMCU/startHour",0);
		Firebase.setInt(database, "/dataApptoMCU/startMinute",0);
		Firebase.setInt(database, "/dataApptoMCU/endHour",0);
		Firebase.setInt(database, "/dataApptoMCU/endMinute",0);
		
		glResetTimerFlag = 0;
	}

	     
}
////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////
/* set Power Pump */
void setPowerLed(unsigned char mode)
{
	digitalWrite(GPIO_NUM_23,mode);
}
////////////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////
/* Check Data */
void queryData()
{
	/* off Pump */
	if(Led == 0 && alarmLed == 0){
		setPowerLed(OffLed);
		statusLed = 0;
		statusAlarm = 0;
		glResetTimerFlag = 0;
		clearData();
  	}
	else if(Led == 0 && alarmLed == 1 && statusAlarm == 0){

		//Serial.print(startMinute);
		//Serial.print(currentTime.Minutes);
    	setPowerLed(OnLed);
		statusLed = 1;
		glResetTimerFlag = 0;

		// Start alarm
		setTime(startHour,startMinute);
	  	statusAlarm = 1;	
	}
	else if(Led == 1 && alarmLed == 0 && statusAlarm == 0){
		setPowerLed(OnLed);
		statusLed = 1;
		glResetTimerFlag = 0;
		
		// Limit time Led = 20 minute 
		setLimitedTime();
		setTime(sHour,sMinute);
		statusAlarm = 1;
		glLimitedTimeFlag = 1;
	}
	else if(Led == 1 && alarmLed == 1 && statusAlarm == 0){
		setPowerLed(OnLed);
		statusLed = 1;
		glResetTimerFlag = 0;

		// Start alarm
		setTime(startHour,startMinute);
		statusAlarm = 1;
	}
		
}
////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////
/* Alarm Pump */
void setTime(unsigned char hour, unsigned char minute)
{
	rtc.setTime(0, minute, hour, 8, 11, 2001);
}


void getTime()
{
	// get time now 
	timeNow.Hours = rtc.getHour((true));
	timeNow.Minutes = rtc.getMinute();
	
}


unsigned char alarm(unsigned char hour, unsigned char minute)
{
	unsigned char flag;

	//Read Time from RTC
	getTime(); 

	if(timeNow.Hours == hour){
		if(timeNow.Minutes < minute){
			flag = 1;     // continnue alarm
		}
		else{
			flag = 0;     // stop alarm
		}
	}
	else{
		flag = 1;         // continnue alarm
	}
	return flag;
}
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
/* Limited time for Pumping */
void setLimitedTime()
{
	sHour = 7;
	sMinute = 0;
	
	if((sMinute + timePlay) < 60){
		eMinute = sMinute + timePlay;
        eHour = sHour;
    } 
	else{
        eMinute = (sMinute + timePlay) - 60;
        eHour = sHour + 1;
    }

}

////////////////////////////////////////////////////////////


/* clear buffer database */
void clearData()
{
	Led = 0;
	alarmLed = 0;
	startHour = 0;
	startMinute = 0;
	endHour = 0;
	endMinute = 0;
	
}


/* check connect wifi */
unsigned char Wifi()
{
	unsigned char flag;
	
	if(WiFi.status() != WL_CONNECTED){
		flag = 0;
	}
	else{
		flag = 1;
	}
	
	return flag;
}

unsigned char toggle(unsigned char a)
{
	if(a == 0){
		a = 1;
	}
	else{
		a = 0;
	}
	return a;
}
