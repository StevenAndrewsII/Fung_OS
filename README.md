# Fung_OS
<p align="center">
<img alt="Static Badge" src="https://img.shields.io/badge/Buy_me_a_coffee-%5E__%5E-blue?link=https%3A%2F%2Fwww.buymeacoffee.com%2FStevenII">
</p>

# About Fung_OS :  

Fung_OS is an open source embeded system to control & monitor multiple enclosed mushroom cultivation envrionemnts for large scale lab environments and production. 
Utilizing an ArduinoMEGA as the backend hardware controller and Rasbery PI 3B+ as the frontside user interface and system manager. 

The system is broken into two parts. The Arduino "Backend" and the PI "frontside". 

The Arduino is resposible for life support(s) managment and its attached environments that are controlled by the life support sub system. 

The Rasbery PI is responsible for the collection of data from the backend as well as providing an easy user interface to manage several systems and subsystems. 


## Arduino ( Backend )

> Definition: The Arduino( backend) is the hardware controller for life support subsystems and their connected environments...

The Arduino backend is witten in C++.  
Each Arduino can control 3 life supports.                  > ( envrionmental control hardware )
                                                          
Each life support can by default, support 4 environments   > ( fruiting chambers / tents / rooms )  

> **Note**
> 
> Environments are isolated Fruiting  [ Chambers / Tents / Rooms ] 

### life support:
Life support subsystem manages:

> 1. Lighting 
> 1. Heating 
> 1. Humidity/Air exchange
> 1. Water tank cleaning 
> 1. Waste pumps
> 1. Door/lighting bypass switch
> 1. Water level sensor
> 1. Valve system control 
    
  
## Rasbery PI

The rasbery pi frontside features:

> 1. User interface  
> 1. Full/Multi system control (multi Arduino communication)
> 1. Wifi - local network cluster communication
> 1. Data logging
> 1. Powerd by Pygame as rendering engine  
> 1. Multithreaded application 


## System communication overview:

Basic example:

Arduino   <-- communication via USB -->  RasberyPI : ( user interface layer ) 

flow chart:
```mermaid
graph TD;
    EnvironmentA_1-8-->Arduino_1-->PI_1;
    EnvironmentB_1-8-->Arduino_2-->PI_1;
    EnvironmentC_1-8-->Arduino_3-->PI_1;
    EnvironmentD_1-8-->Arduino_4-->PI_1;
```


## Life support sub_system overview:

Basic example: ( environments can be assigned to any life support ! )

life support (A) ---> FC_1, FC_2, FC_3

life support (B) ---> FC_4, FC_5

flow chart:
```mermaid
graph TD;
    Environment_A-->Life_support_1-->Arduino_1;
    Environment_B-->Life_support_1;
    Environment_C-->Life_support_1;
    Environment_D-->Life_support_2-->Arduino_1;
    Environment_E-->Life_support_2;
    Environment_F-->Life_support_2;
    Arduino_1-->PI_1;
  
```



> **Note**

> **Warning**


<details>
<summary> Click to expand </summary>
  
1. hidden a
2. hidden b

</details>
