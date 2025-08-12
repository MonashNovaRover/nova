# pivot_drive_controller

This controller implements a pivot drive model of control, where all wheels turn to be tangent towards the turning point. This will cause the wheels of the vehicle to trace a circular path around the turning point.

You can find the kinematic functions pertaining to this drive model in `kinematics.hpp`.

## Glossary

### Zero Radius
The *zero radius* is the radius of the circle the wheels of the rover makes when turning on the spot, i.e. the distance between the wheels to the center of the rover since the wheels are equidistant.

### Inner Radius
I'm not entirely sure on the theory/math behind this and this was figured out mostly through empirical observations, but wheel speed must be calculated differently once the turning radius is smaller than the radius of the circle made by the wheel on the side of the turn.

I've named the turning radius at which the radius of the circle made by the wheel on the side of the turn is equal the *inner radius*. When the turning radius is smaller than the turning radius, the speed is calculated based on the *zero radius*.

## Notable Paramaters:
- `pivot_rate_tolerance` - in autonomous mode, we may want to delay movement until pivots have reached the desired angle. This parameter controls **how long in seconds** before the pivots reach their target positions to start moving. Only affects autonomous mode operation.