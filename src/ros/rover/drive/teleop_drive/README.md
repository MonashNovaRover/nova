# teleop_drive

A teleop package for drive.


Written with [teleop_modular](https://github.com/BaileyChessum/teleop_modular). This package created with created with [teleop_template](https://github.com/BaileyChessum/teleop_template). 

### Example

Out of the box, this package runs turtlesim, controlled by a game controller:

```shell
ros2 launch A teleop package for drive. teleop.launch.py
```

This is just silly minimal demo. Please replace it with something meaningful, and expand it to your preference. You can add multiple launch files and parameter files. You might wish to split input source parameters across different parameters to support launching different input sources, for example.

Refer to the [Teleop Modular Documentation](https://baileychessum.github.io/teleop_modular/index.html)!

**Controls**

- Left stick to drive forward and back
- Right stick to steer
- Right bumper to enter 'turbo mode'

---

#### TODO: 
- [ ] Remove turtle sim from [teleop.launch.py](./launch/teleop.launch.py)
- [ ] Write control mode packages for your robot, and add them to [teleop.yaml](./params/teleop.yaml).
- [ ] Write a param file for an input source, make a control scheme with [remap params](https://baileychessum.github.io/teleop_modular/about/input_source_remapping.html), and import it in [teleop.launch.py](./launch/teleop.launch.py).
 