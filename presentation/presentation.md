---
marp: true
theme: gaia
class: invert
size: 16:9

transition: cover
math: mathjax
---

# <span style="color:#FFA07A;">Power Meter with </span>
# <span style="color:#FFA07A;">Protection & Visualization</span>
#### Introduction
#### Circuit design
#### Firmware design
#### App design
#### Result
---

# <span style="color:#FFA07A;">Introduction</span>
- Importance of power monitor in fields such as medical, robotics, renewable, etc.
    - Ensure correct operating ranges. 
    <span style="color:#ADFF2F;">$\rightarrow$ Configurable ranges at run-time.</span>
    - Visualizatoin of power/current consumption.
    <span style="color:#ADFF2F;">$\rightarrow$ App run on client's PC/laptop.
    - Protection against damage.</span>
    <span style="color:#ADFF2F;">$\rightarrow$ Electronically controlled switch - MOSFET.</span>

---
# <span style="color:#FFA07A;">Circuit Design</span>
#### MOSFET selection
#### MOSFET gate driver
#### Power rail
#### Latch and overvoltage detection
#### Current sensor and MCU

--- 
## <span style="color:#FFA07A;">MOSFET selection</span>
##### Switching condition
- For N-channel MOSFET (NMOS): $\quad V_{gs}>V_{th}, \quad V_{th} > 0V$
$\rightarrow$ Low-side switch

![height: 400px, width:400px](./media/nmos_switch.png)

--- 
## <span style="color:#FFA07A;">MOSFET selection</span>
##### Switching condition
- For P-channel MOSFET (PMOS): $\quad V_{gs}<V_{th}, \quad V_{th} < 0V$
$\rightarrow$ High-side switch
![height: 400px, width:400px](./media/pmos_switch.png)

--- 
## <span style="color:#FFA07A;">MOSFET selection</span>
##### On resistance $R_{ds}$
- NMOS has lower $R_{ds}$ than PMOS
- PMOS can be paralleled to reduce the total $\sum R_{ds}$

--- 
## <span style="color:#FFA07A;">MOSFET selection</span>
##### Gate charge
- There exist parasitic capacitors when switching MOSFET.
![width:400px](./media/mosfet_para_C.png)
- The total turn-on charge is: $Q_{on} = -[(C_{gs} +C_{gd})\cdot V_{gs, miller} - V_{signal}\cdot C_{gd}]$
- For quick prototyping, the value $Q_g$ in the datasheet can be used.

--- 
## <span style="color:#FFA07A;">MOSFET selection</span>
##### Safe Operating Area (SOA)
![](./media/SOA_regions.drawio.png)

--- 
## <span style="color:#FFA07A;">MOSFET selection</span>
##### Safe Operating Area (SOA) - Region I
- Ohmic region: $V_{ds}=R_{ds}\cdot I_d$. 
- Spanning from $V_{ds}=0V$ to $V_{ds}=\dfrac{I_{d,\text{max}}}{R_{ds}}$
![](./media/SOA_regions_presentation.drawio.png)

--- 
## <span style="color:#FFA07A;">MOSFET selection</span>
##### Safe Operating Area (SOA) - Region II
- Limit by package, silicon, and $R_{\theta JA}$.
- Spanning from $V_{ds}=\dfrac{I_{d,\text{max}}}{R_{ds}}$ to $V_{ds}=\dfrac{P_{max}}{I_{d,\text{max}}}$.
![](./media/SOA_regions_presentation.drawio.png)

--- 
## <span style="color:#FFA07A;">MOSFET selection</span>
##### Safe Operating Area (SOA) - Region III & IV
- Region III: limited by $P_{\text{max}}$.
- Region IV: Limited by thermal instability.
![](./media/SOA_regions_presentation.drawio.png)

--- 
## <span style="color:#FFA07A;">MOSFET selection</span>
##### Safe Operating Area (SOA) - Design guide
Region II, III, IV is <span style="color:#ADFF2F;">**soft-limited**</span>
$\rightarrow$ select to be well above desired operation.
- $V_{dss} \geq 1.2\cdot V_{in}$.
- $I_{d,\text{max}} \geq 1.2\cdot I_{in}$.
- Low on resistance $R_{ds}$.

--- 
## <span style="color:#FFA07A;">MOSFET selection</span>
##### Summary
- SOA is guaranteed.
- Parasitic capacitance does not vary largely.


&emsp;&emsp;&emsp;&emsp;&emsp;&emsp; PMOS &emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;NMOS
Switch&emsp;&emsp;&emsp;High-side&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;-Low-side$\rightarrow$ **ground offset**
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&nbsp; -Can be used as high-side
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&nbsp; but more complex

$R_{ds}$&emsp;&emsp;&emsp;&emsp;&nbsp; High$\rightarrow$ improve by&emsp;&emsp;&emsp;&emsp;Low
&emsp;&emsp;&emsp;&emsp;&emsp;&emsp; paralleling

--- 
## <span style="color:#FFA07A;">MOSFET selection</span>
##### IRF4905S
The following MOSFET is chosen beacause:
- suitable $V_{dss}$ and $I_{d,\text{max}}$.
- low $R_{ds}$ among PMOS.

![](./media/IRF4905_key_parameters.png)

--- 
## <span style="color:#FFA07A;">MOSFET selection</span>
##### Reverse polarity protection
Create by connecting Drain to $V_{in}$ and Source to load.
- The body diode (N-Type Substrate and P-Type Drain) allows a current path if $V_{d} > V_{s}$.
- When a reverse polarity is applied, $V_{gs}>0V$, MOSFET in cut-off.

![width=200px](./media/reverse_voltage_protection.png)

---
## <span style="color:#FFA07A;">MOSFET gate driver</span>
##### Objective
- Slow turn-on & quick turn-off.
- Driven in deep saturation, $V_{gs} \approx -V_{in}$

---
## <span style="color:#FFA07A;">MOSFET gate driver</span>
##### A naive approach:
![width:800px](./media/pmos_switch_npn.png)

---
## <span style="color:#FFA07A;">MOSFET gate driver</span>
##### Proposed gate driver
![width:600px](./media/proposed_gate_driver.png)

---
## <span style="color:#FFA07A;">MOSFET gate driver</span>
##### Proposed gate driver
On HIGH input, BJT $T2$ is saturated, pulling $R6$ to ground, cutting off $T1$. Thus, discharging the MOSFET's gate through $R5$.
![width:600px](./media/proposed_driver_ON.png)

---
## <span style="color:#FFA07A;">MOSFET gate driver</span>
##### Proposed gate driver
On LOW input, BJT $T2$ is cut off, current flows through $R6$ to the bas off $T1$, turning it on. Thus, charging the MOSFET's gate.
![width:600px](./media/proposed_driver_OFF.png)

---
## <span style="color:#FFA07A;">Power Rail</span>
- Hyrbid approach of switching LM2596 and LDO L7805 regulator.
- Use Isolated DC-DC Converter B0505S-2WR2 to power the MCU.

![width:800px](./media/power_rail_schematic.png)

---
## <span style="color:#FFA07A;">Latch circuit</span>
- The following diagram shows the logic of the desired latching behaviour.
- If design with logic ICs, at least 4 ICs, and unused gates

![width:700px](./media/latch_circuit_diagram.png)

---
## <span style="color:#FFA07A;">Latch circuit</span>
##### Single op-amp latch design

![width:900px](./media/op_amp_latch_circuit.png)

---
## <span style="color:#FFA07A;">Latch circuit</span>
##### Single op-amp latch design

![height:480px](./media/latch_circuit_ON.png)

---
## <span style="color:#FFA07A;">Current sensor INA226</span>
##### Features
- All-in-one solution for power monitor application.
- Shunt-based with wide measurement range $[-81.975mV; 81.92mV]$ & 16 bits ADC.
- I2C communication.
- Programmable Alert function and limit with output on pin ALE.

---
## <span style="color:#FFA07A;">Current sensor INA226</span>
##### Setup
- High-side measurement.
- Bus voltage measured at load $\rightarrow$ VBUS and IN- shorted.
- Alert function monitors power, outputs if set limit is exceeded.
![bg right:33%](./media/INA226_board.png)

---
## <span style="color:#FFA07A;">ESP32-C3 development board</span>
##### Features
- UART-to-USB converter and LDO converter.
- On-board antenna for WiFi connectivity.
- On-board RGB LED (tied to GPIO3, GPIO4, GPIO5).

![bg right:50%](./media/esp_board.png)

---
## <span style="color:#FFA07A;">ESP32-C3 development board</span>
##### Digital isolation
To use with other components such as the latch circuit and INA226, a digital isolator IC and an I2C isolator IC are required.
- **ADUM3201ARZ** from Analog Devices Inc.
- **ISO1540** from Texas Instruments Inc

---
## <span style="color:#FFA07A;">PCB layout</span>
##### Thermal via for TO-263/D2PAK

![width:900px](./media/d2pak_thermal_via.png)

---
## <span style="color:#FFA07A;">PCB layout</span>
##### Amass XT60PW connectors
- Directional.
- Anti-spark.
- Pressed fit connection.

![bg right:50%](./media/XT60PW.png)

---
## <span style="color:#FFA07A;">Firmware Design</span>
##### Setup
![width:1200px](./media/main_program_flowchart.drawio.png)

---
## <span style="color:#FFA07A;">Firmware Design</span>
##### read_sensor_task
![width:1200px](./media/read_sensor_task_flowchart.drawio.png)

---
## <span style="color:#FFA07A;">Firmware Design</span>
##### send_data_task
![width:1200px](./media/send_data_task_flowchart.drawio.png)

---
## <span style="color:#FFA07A;">Firmware Design</span>
##### check_user_input_task
![width:800px](./media/check_user_input_task_flowchart.drawio.png)

---
## <span style="color:#FFA07A;">Application</span>
##### Model-View-Controller
![width:1000px](./media/mvc_modified.drawio.png)

---
## <span style="color:#FFA07A;">Application</span>
##### Framework
![bg right](./media/flask_celery_bg.png)
- The main framework is **Flask**.
- **Celery** is used to run background task, capturing incoming UDP packets and store for visualization.

---
## <span style="color:#FFA07A;">Application</span>
##### Controller's behaviour
The controller will wait for the following requests:
- HTTP GET **/esp32_post**: Collect all data points up until request, and send to client.
- HTTP POST **/sensor_config**: Encode & "redirect" to the MCU.
- HTTP POST **/switch_off**:    "redirect" to the MCU.

---
## <span style="color:#FFA07A;">Application</span>
##### View's design
- **Bootstrap 5** provides elements to design a modern UI.
- **Plotly.js** is used to graph the data.
- **AJAX** allows for update of elements without reloading the webpage.
![bg right:40%](./media/view_js_lib.png)

---
## <span style="color:#FFA07A;">Application</span>
##### View's design - index
![width=300px](./media/index_html.png)

---
## <span style="color:#FFA07A;">Application</span>
##### View's design - config
![width=300px](./media/config_html.png)

---
# <span style="color:#FFA07A;">Result</span>
#### Gate driver's transient
#### Power rail's component failure
#### Latch circuit
#### Firmware and Application

---
## <span style="color:#FFA07A;">Result</span>
##### Gate driver's transient
The following is the voltage traces of the drain voltage (green) and the gate (yellow) voltage of the MOSFET during a turn-off event.
![width=300px](./media/turn_off_no_load.png).

---
## <span style="color:#FFA07A;">Result</span>
##### Gate driver's transient
Recall the gate capacitance calculation, $V_{gs,miller}$ can be calculated:
$$V_{gs, miller} = V_{signal}\left[\exp{\left(-\dfrac{\Delta t_{0\rightarrow2}}{R_{g}(C_{gs} +C_{gd})}\right)}\right]$$
- In the datasheet, $C_{gs} +C_{gd} = C_{iss}(V_{ds}) = 1448pF$
- $V_{gs, miller}=V_{g, miller} - 15V$
- In the proposed gate driver topoloy, the gate is pull-up to supply voltage, thus, $V_{signal} = V_{in} = 15V$.

---
## <span style="color:#FFA07A;">Result</span>
##### Gate driver's transient
$\qquad V_{g, miller} - 15V = V_{signal}\left[\exp{\left(-\dfrac{\Delta t_{0\rightarrow2}}{100k\Omega\cdot 1448pF}\right)}\right]$
$\Leftrightarrow V_{g, miller} = V_{signal}\left[\exp{\left(-\Delta t_{0\rightarrow2}\cdot 6900 s^{-1}\right)}\right] + 15V \geq 15V,\quad \forall t_{0\rightarrow2}$
Let:
$\qquad\qquad \Delta t_{0\rightarrow2} \in [100\mu s;200\mu s] \Leftrightarrow V_{g, miller} \in [1.25V_{in}; 1.5V_{in}]$

The above range for $V_{g,miller}$ agrees with the measured response.

**Solution:** snubber circuit to decrease $dv/dt$ of $V_{ds}$ at device turn-off.

---
## <span style="color:#FFA07A;">Result</span>
##### Power rail's components failure
- LM2596 was not sourced from trusted vendor causes fluctuating $V_{ref\_LM2596} \in [1.0V; 1.23V]$ leads to:
    - Fluctuating $V_{out,sw}$.
    - Higher $P_{loss}$ at the LDO stage.
- B0505S-2WR2 cannot maintain 5V output when the MCU is connected.
$\rightarrow$ The MCU has to be powered externally.

---
## <span style="color:#FFA07A;">Result</span>
##### Latch circuit
- Overlapping of on and off source was not accounted for.
- Possible solution:
![width=200px](./media/turn_on_warning.drawio.png)