# OpenBox Service Instance
An OpenBox software-based service instance implementation. Since this project is under ongoing development, not all sources are available in this repository.

# Installation

Clone this repo (assuming it was cloned into the $HOME directory), run the following commands:  
`cd ~/obsi`        
`sudo ./install.sh`    

Note: The installation script might prompt you to install additional packages.
Note: The install script installs both Netmap and DPDK on the system.

# Running

(assuming it was cloned into the $HOME directory):

`cd ~/obsi`
`./activate.sh [kernel | netmap | dpdk] <interfaces...>`
`cd ~/obsi/openbox`  
`vim config.py (and adjust any configuration parameters)`    
`sudo python manager.py` or `./run` at openbox directory can be used  

If you want to run against a mock controller:  
`cd ~/obsi/tests`  
`python mock_controller.py`  

# Stopping
Just kill the process and the 2 subprocess it creates:  
`ps -fade | grep python`  
`sudo kill -9 <PID>`  

# Auther
Pavel Lazar (pavel.lazar (at) gmail.com) and Shlomo Shenzis (shlomo.shenzis@mail.huji.ac.il)
