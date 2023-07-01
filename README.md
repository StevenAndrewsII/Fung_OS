# Fung_OS
<p align="center">
<img alt="Static Badge" src="https://img.shields.io/badge/Buy_me_a_coffee-%5E__%5E-blue?link=https%3A%2F%2Fwww.buymeacoffee.com%2FStevenII">
</p>

About Fung OS :  

Fung_OS is an open source embeded system to controll & monitor multiple enclosed mushroom cultivation envrionemnts for large scale production. 
Utilizing an ArduinoMEGA as the backend hardware controller and Rasbery PI 3B+ as the frontside user interface and system manager. 

The system is broken into two parts. The Arduino "Backend" and the PI "frontside". 

The Arduino is resposible for life support(s) managment and its attached environments that are supported by the life support sub system. 

The rasbery pi is responsible for the collection of data from the backend as well as providing an easy user interface to manage several systems and subsystems. 

## Arduino ( Backend )

The Arduino backend is witten in C++.  
Each Arduino can control 3 life supports.                  > ( envrionmental control hardware )
                                                          
Each life support can by default, support 4 environments   > ( fruiting chambers / tents / rooms )  

> **Note**
> 
> Environments are isolated Fruiting chambers or tents.

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






> **Note**

> **Warning**


<details>
<summary> Click to expand </summary>
  
1. hidden a
2. hidden b

</details>
