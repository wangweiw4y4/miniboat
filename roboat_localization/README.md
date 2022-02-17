# Roboat Localization

<p align='center'>
    <img src="./launch/demo.png" alt="drawing" width="800"/>
</p>

## Package

This package fuses pose information from multiple sensors for roboat localization.

## Run

```
roslaunch roboat_localization run.launch
```

## Topics

  - ```roboat_localization/odometry_ekf/odometry_filtered```: EKF odometry topic
  - ```roboat_localization/odometry_ekf/accel_filtered```: EKF acceleration topic

## Parameters

  - EKF parameters are defined in ```config/params.yaml```

## Copyright
```
Copyright 2016-2021 Roboat, all rights reserved.
Private software. No distribution allowed.
```