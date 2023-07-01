# Fung_OS
<p align="center">
<img alt="Static Badge" src="https://img.shields.io/badge/Buy_me_a_coffee-%5E__%5E-blue?link=https%3A%2F%2Fwww.buymeacoffee.com%2FStevenII">
</p>

About Fung OS :  

Fung_OS is an embeded system to controll & monitor multiple enclosed mushroom cultivation envrionemnts. 
Utilizing an ArduinoMEGA as the backend hardware controller and Rasbery PI 3B+ as the frontside user interface and system manager. 

The system is broken into two parts. The Arduino "Backend" and the PI "frontside". 

The Arduino is resposible for life support and the enviroments that are supported by the life support sub system. 

The rasbery pi is responsible for the collection of data from the backend as well as providing an easy user interface to manage several systems and subsystems. 

## Arduino 


## Rasbery PI

Here is a simple flow chart:

```mermaid
graph TD;
    EnvironmentA_1-8-->Arduino_1-->PI_1;
    EnvironmentB_1-8-->Arduino_2-->PI_1;
    EnvironmentC_1-8-->Arduino_3-->PI_1;
    EnvironmentD_1-8-->Arduino_4-->PI_1;
```


Arduino   <-- communication via USB -->  RasberyPI : ( user interface layer ) 



> **Note**

> **Warning**


<details>
<summary> Click to expand </summary>
  
1. hidden a
2. hidden b

</details>
