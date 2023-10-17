from rockit import *
from casadi import *
import numpy as np

# Define problem parameters
#boat_diam = 0.30
max_speed_limit = 0.04

## Problem parameters
nx    = 2                   # the system is composed of 2 states per robot
Tf    = 1                   # control horizon [s]
Nhor  = 1                  # number of control intervals
#dt    = Tf/Nhor             # sample time
number_of_robots = 7        # number of robots that are neighbors (without local)

#mu = 1

## Create OCP object
ocpC = Ocp(T=Tf)
#ocpZ = Ocp(T=Tf)

## Variables of own robot
#z_x = ocpZ.variable(grid='control',include_last=True)
#z_y = ocpZ.variable(grid='control',include_last=True)
#Z = vertcat(z_x,z_y)

u_cbf = ocpC.variable(grid='control',include_last=True)
v_cbf = ocpC.variable(grid='control',include_last=True)

boat_diam = ocpC.register_parameter(MX.sym('boat_diam', 1))
u_mpc = ocpC.register_parameter(MX.sym('u_mpc', 1))
v_mpc = ocpC.register_parameter(MX.sym('v_mpc', 1))
x = ocpC.register_parameter(MX.sym('x', 1))
y = ocpC.register_parameter(MX.sym('y', 1))

# Parameters for neighbors
#C_j = ocpC.register_parameter(MX.sym('C_j', nx*number_of_robots), 1)
C_j = ocpC.parameter(number_of_robots*nx)
#V_j = ocpC.parameter(number_of_robots*nx)

ocpC.add_objective(ocpC.sum((u_cbf - u_mpc)**2 + (v_cbf - v_mpc)**2))
ocpC.subject_to( (-max_speed_limit <= u_cbf) <= max_speed_limit )
ocpC.subject_to( (-max_speed_limit <= v_cbf) <= max_speed_limit )

for j in range(number_of_robots):
    cx_j = C_j[2*j]
    cy_j = C_j[2*j+1]
    #vx_j = V_j[2*j]
    #vy_j = V_j[2*j+1]
    #cbf_j = (2*(cx_j-x)*(vx_j-u_cbf) + 2*(cy_j-y)*(vy_j-v_cbf)) + ((cx_j-x)**2 + (cy_j-y)**2 - boat_diam**2)
    cbf_j = (2*(cx_j-x)*(-u_cbf) + 2*(cy_j-y)*(-v_cbf)) + ((cx_j-x)**2 + (cy_j-y)**2 - (boat_diam)**2)
    ocpC.subject_to( cbf_j >= 0 )
    
options = {"ipopt": {
    "print_level": 3,
    # "linear_solver": "ma27",
    # "tol": 1e-6,
    'sb': 'yes',
    }}
options["expand"] = True
options["print_time"] = False
ocpC.solver('ipopt',options)
ocpC.method(MultipleShooting(N=Nhor,M=1,intg='rk'))


## Set dummies for parameter values (required for ocp.to_function)
## In case you create the OCP function without solving the OCP first
ocpC.set_value(u_mpc, 0.3)
ocpC.set_value(v_mpc, 0.3)
ocpC.set_value(x, 0.0)
ocpC.set_value(y, 0.0)
ocpC.set_value(boat_diam, 0.3)

#####################################
## Define values for testing
#####################################
C_values = np.zeros([nx*number_of_robots])
for j in range(number_of_robots):
    C_values[2*j] = j*10+10
    C_values[2*j+1] = j*10+10
V_values = np.zeros([nx*number_of_robots])

#####################################
## Solve OCP with from OCP object
#####################################
## Set parameters
ocpC.set_value(C_j, C_values)
#ocpC.set_value(V_j, V_values)

## Execute the solver
ocpC.solve()


#########################################
## Create CasADi function from OCP object
#########################################
## Sample variables to get their symbolic representation
_, uc_samp = ocpC.sample(u_cbf, grid='control')
_, vc_samp = ocpC.sample(v_mpc, grid='control')

boat_diam_samp = ocpC.value(boat_diam)
x_samp = ocpC.value(x)
y_samp = ocpC.value(y)
um_samp = ocpC.value(u_mpc)
vm_samp = ocpC.value(v_mpc)
Cj_samp = ocpC.value(C_j)
#Vj_samp = ocpC.value(V_j)

## Define inputs and outputs of CasADi function
parameters = [boat_diam_samp, x_samp, y_samp, um_samp, vm_samp, Cj_samp]#, Vj_samp]

inputs = parameters
input_names = ['diam_samp', 'x_samp', 'y_samp', 'umpc_samp', 'vmpc_samp', 'Cj_samp']#, 'Vj_samp']

outputs = [uc_samp, vc_samp]
output_names = ['u_cbf', 'v_cbf']

## Create CasADi function
ocp_function = ocpC.to_function('ocpCBF', 
                                inputs,
                                outputs, 
                                input_names,
                                output_names)

## Serialize function
ocp_function.save('ocpCBF.casadi')

## Test
ocp_function(0.3, 0.0, 0.0, 0.0, 0.0, C_values)#, V_values)