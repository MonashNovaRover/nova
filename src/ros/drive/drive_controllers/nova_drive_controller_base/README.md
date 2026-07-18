# nova_drive_controller_base

This package provides a base for all drive controllers written for the rover.

## Usage
To inherit from `NovaDriveControllerBase`, you must implement the following methods:
- `void init_params()` - if your controller specifies it's own params in addition to the base params provided by `NovaDriveControllerBase`, you should initialise the `ParamListener` and `Params` in this method. This is also where you initialise any variables that depend on parameters.
- `void update_params()` - should be the same for all controllers, just check if your specific parameters are outdated and update them if so. Even though this is the same for all derived classes, it's not in the base class because to do so we would need to have access to derived class' params. In theory this is doable using CRTP (Curiously Recurring Template Pattern), but this pattern is a bit esoteric.
- `void reset_limiter_buffers()` - where you reset limiter buffers you defined such as `previous_speeds_`. Pretty straightforward.
- `Commands twist_to_commands(const geometry_msgs::msg::Twist& twist_msg, bool autonomous_mode, const rclcpp::Duration& period)` - main method you need to implement. This is where you convert Twist input into commands and data to be sent to hardware interfaces and be used in odometry respectively.

## Notes
- Even if your controller doesn't have it's own params, you still need to include `generate_parameter_library` as a dependency in your `CMakeLists.txt`, `package.xml` and `default.nix` because `NovaDriveControllerBase` uses it.
- You may notice that existing controllers specify `controller_interface::ControllerInterface` as the base class with `pluginlib`. This is because `controller_manager` only looks for plugins registered with controller_interface when it tries to load plugins. It doesn't care what class your controller actually inherits from, as long as it's an intermediate class i.e. ControllerInterface -> CustomBaseClass -> YourController.
<details>
<summary>Regarding pluginlib, here's a wonderful explanation by our good friend ChatGPT:</summary>

## 1️⃣ What pluginlib actually does

When you call in **CMake**:

```cmake
pluginlib_export_plugin_description_file(controller_interface my_controller.xml)
```

it:

* Copies your `my_controller.xml` into the install space.
* Registers it in the **ament index** under the *resource type*:

```
controller_interface__pluginlib__plugin
```

That `controller_interface` part comes directly from the **first argument** you pass to `pluginlib_export_plugin_description_file()`.

---

## 2️⃣ How controller\_manager discovers controllers

When `controller_manager` starts, it:

1. Asks pluginlib:

> “Show me all plugins registered under the base class `controller_interface::ControllerInterface`”
2. `pluginlib` looks up all packages that have exported a file under the index:

```
controller_interface__pluginlib__plugin
```

3. It parses each XML found there and checks for `<class>` entries where the `base_class_type` string matches exactly the C++ type name it expects.

So the only things `controller_manager` will ever see are:

* XML files installed in packages that have registered themselves in the `controller_interface` plugin index.
* Classes in those XML files whose `base_class_type` string matches what `controller_manager` asked for.

---

## 3️⃣ Why NovaDriveControllerBase didn’t work

`NovaDriveControllerBase` is your own base class in `nova_drive_controller_base`, but:

* You never called

```cmake
pluginlib_export_plugin_description_file(nova_drive_controller_base something.xml)
```

in that package, so **there’s no `nova_drive_controller_base__pluginlib__plugin` index**.
* Even if you did, `controller_manager` never queries that index — it only queries `controller_interface__pluginlib__plugin`.
* And `pluginlib` doesn’t “know” that `NovaDriveControllerBase` inherits from `ControllerInterface` — it only does string equality checks.

So if your XML says:

```xml
base_class_type="nova_drive_controller_base::NovaDriveControllerBase"
```

and you export it under `nova_drive_controller_base`, the file lives in the wrong index and `controller_manager` won’t look there.

---

## 4️⃣ The takeaway

* **The first argument** to `pluginlib_export_plugin_description_file()` chooses which *index* the plugin goes in.
* **The `base_class_type` in the XML** must match exactly the C++ type you register in `CLASS_LOADER_REGISTER_CLASS()`.
* The loader (here, `controller_manager`) only looks in one index: the one for `controller_interface`.

That’s why the fix was:

* XML `base_class_type="controller_interface::ControllerInterface"`
* `CLASS_LOADER_REGISTER_CLASS(..., controller_interface::ControllerInterface)`
* `pluginlib_export_plugin_description_file(controller_interface pivot_drive_controller.xml)`

Because `pluginlib` doesn’t actually care about the **real C++ inheritance chain** — it only cares about two things that are just strings, not types:

1. **The resource index name** (comes from the first argument to `pluginlib_export_plugin_description_file()` in CMake).
2. **The `base_class_type` string in the XML** (must match exactly the second argument in `CLASS_LOADER_REGISTER_CLASS()` in C++).

---

### How this plays out

When `controller_manager` loads controllers:

1. It calls pluginlib with:

```cpp
pluginlib::ClassLoader<controller_interface::ControllerInterface> loader("controller_interface", "controller_interface::ControllerInterface");
```
2. Pluginlib looks at the **resource index**:

```
controller_interface__pluginlib__plugin
```

and gathers all XML files there.
3. It parses each `<class>` tag and checks:

> “Does `base_class_type` match exactly `"controller_interface::ControllerInterface"`?”

If yes → it will `dlopen()` the `.so` and try to `new` the class you registered.

---

### Why the *actual* C++ base doesn’t matter to pluginlib

If you write:

```cpp
CLASS_LOADER_REGISTER_CLASS(MyController, controller_interface::ControllerInterface)
```

pluginlib records only:

* The **string** `"MyController"` for the class name.
* The **string** `"controller_interface::ControllerInterface"` for the base class.

It does not use RTTI or check that `MyController` truly inherits from `controller_interface::ControllerInterface` — that’s your responsibility.

So if your class *really* inherits through a chain:

```
PivotDriveController → NovaDriveControllerBase → ControllerInterface
```

you can still register it as:

```cpp
CLASS_LOADER_REGISTER_CLASS(PivotDriveController, controller_interface::ControllerInterface)
```

because pluginlib just stores that mapping in its registry.

---

**In short:**
The loader (`controller_manager`) is hard-coded to look for plugins indexed under `controller_interface` with a base class string `"controller_interface::ControllerInterface"`.
As long as you tell pluginlib “treat my class as if it were a direct `ControllerInterface` plugin,” it works — even if there’s an intermediate base in real C++ inheritance.
</details>