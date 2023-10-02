from rockit import *
from casadi import *
import numpy as np

# Define problem parameters
max_speed_limit = 0.04

# Problem parameters
nx    = 2                   # the system is composed of 2 states per robot
nu    = 2                   # the system has 2 inputs per robot
Tf    = 2                   # control horizon [s]
Nhor  = 10                  # number of control intervals
dt    = Tf/Nhor             # sample time

mu = 1

# Create OCP object
ocpX = Ocp(T=Tf)

# States
x = ocpX.state()
y = ocpX.state()
X = vertcat(x,y)

# Controls
u = ocpX.control()
v = ocpX.control()

# Derivatives
ocpX.set_der(x, u)
ocpX.set_der(y, v)

# Reference parameters
x_ref = ocpX.register_parameter(MX.sym('x_ref', 1))
y_ref = ocpX.register_parameter(MX.sym('y_ref', 1))

# Lagrange objective
ocpX.add_objective(ocpX.sum((x_ref-x)**2 + (y_ref-y)**2))
ocpX.add_objective(ocpX.at_tf(10*(x_ref-x)**2 + 10*(y_ref-y)**2))
ocpX.add_objective(ocpX.sum(u**2 + v**2, include_last=True))
ocpX.subject_to( (-max_speed_limit <= u) <= max_speed_limit )
ocpX.subject_to( (-max_speed_limit <= v) <= max_speed_limit )

# Initial condition
X_0 = ocpX.register_parameter(MX.sym('X_0', nx))
ocpX.subject_to(ocpX.at_t0(X)==X_0)

options = {"ipopt": {
    "print_level": 3,
    # "linear_solver": "ma27",
    # "tol": 1e-6,
    'sb': 'yes',
    }}
options["expand"] = True
options["print_time"] = False
ocpX.solver('ipopt',options)
ocpX.method(MultipleShooting(N=Nhor,M=1,intg='rk'))

#ocpX._method.add_sampler("u", u)
#ocpX._method.add_sampler("v", v)

## Set dummies for parameter values (required for ocp.to_function)
## In case you create the OCP function without solving the OCP first
ocpX.set_value(x_ref, 0)
ocpX.set_value(y_ref, 0)
ocpX.set_value(X_0, [0, 0])


#####################################
## Solve OCP with from OCP object
#####################################
## Set parameters
ocpX.set_value(x_ref, 1.0)
ocpX.set_value(y_ref, 1.0)
ocpX.set_value(X_0, [0, 0])

## Execute the solver
ocpX.solve()

#########################################
## Create CasADi function from OCP object
#########################################
## Sample variables to get their symbolic representation
xref_samp = ocpX.value(x_ref)
yref_samp = ocpX.value(y_ref)
X_0_samp = ocpX.value(X_0)

_, x_samp = ocpX.sample(x, grid='control')
_, y_samp = ocpX.sample(y, grid='control')
_, u_samp = ocpX.sample(u, grid='control-')
_, v_samp = ocpX.sample(v, grid='control-')

## Define inputs and outputs of CasADi function
inputs = [xref_samp, yref_samp, X_0_samp]
input_names = ['xref_samp', 'yref_samp', 'X_0_samp']

outputs = [x_samp, y_samp, u_samp, v_samp]
output_names = ['x_samp', 'y_samp', 'u_samp', 'v_samp']

## Create CasADi function
ocpX_function = ocpX.to_function('mpc', 
                                inputs,
                                outputs, 
                                input_names,
                                output_names)

## Serialize function
ocpX_function.save('mpc.casadi')

## Test
ocpX_function(1.0, 1.0, [0, 0])


######################################
# Get discrate dynamics for simulation
######################################
# Dynamics declaration
Sim_asv_dyn = ocpX._method.discrete_system(ocpX)

# Sim_asv_dyn.save('sim_asv_dyn.casadi')