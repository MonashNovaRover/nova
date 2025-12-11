// import { useEffect } from "react";
// import { useSelector } from "react-redux";
// import { RootState } from "../../../redux/RootState.ts";
// import { RosTopic } from "../../../ros/topics/rosTopic.ts";
// import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
// import BatteryGauge from 'react-battery-gauge'
import {Button, Autocomplete, AutocompleteItem, Popover, PopoverTrigger, PopoverContent} from "@nextui-org/react";
// import {Autocomplete, AutocompleteSection, AutocompleteItem} from "@nextui-org/react";

export const Tasks = [
  {label: "Autonomous", key: "auto"},
  {label: "Post-Landing", key: "post-landing"},
  {label: "Lunar Resources", key: "lunar-resources"},
  {label: "Excavation and Construction", key: "excavation-construction"},
  {label: "Science", key: "science"},
  {label: "Retrieval and Delivery", key: "retrieval-delivery"},
  {label: "Equipment Servicing", key: "equipment-servicing"}
]

export const  BatteryWidget = () => {
  return (
    <Popover placement="bottom">
      <PopoverTrigger>
        <Button>Battery Telemetry</Button>
      </PopoverTrigger>
      <PopoverContent>
        <div className="px-1 py-2">
          <Button variant="bordered">Start Tracking Power Usage</Button>
          <Button variant="bordered">Stop Tracking Power Usage</Button>
          <Autocomplete
            className="max-w-xs"
            defaultItems={Tasks}
            label="Task"
            placeholder="Search a task to track"
          >
            {(task) => <AutocompleteItem key={task.key}>{task.label}</AutocompleteItem>}
          </Autocomplete>
        </div>
      </PopoverContent>
    </Popover>

  );
}


// export const BatteryWidget = () => {
//   return (
//     <Dropdown>
//       <DropdownTrigger>
//         <Button variant="bordered">Battery Telemetry</Button>
//       </DropdownTrigger>

//       <DropdownMenu aria-label="Static Actions">
//         <DropdownItem key="start_tracking">Start Tracking Power Usage</DropdownItem>
//         <DropdownItem key="stop_tracking">Stop Tracking Power Usage</DropdownItem>
//         <Autocomplete
//           className="max-w-xs"
//           defaultItems={Tasks}
//           label="Choose a task to track"
//           placeholder="Search for a task"
//         >
//           {(task) => <AutocompleteItem key={task.key}>{task.label}</AutocompleteItem>}
//         </Autocomplete>
//       </DropdownMenu>
//     </Dropdown>
//   );
// }


// export const BatteryWidget = () => {
  
//   const batteryLevel = useSelector((state: RootState) => state.batteryStore.percentage);
//   const bifrost = useBifrost({ topic: RosTopic.BATTERY_STATE });

//   useEffect(() => {
//     bifrost.syncWithTopic();
//   }, [bifrost]);

//   return (
//     <BatteryGauge size={35} value={batteryLevel} customization={{
//       batteryBody: { cornerRadius: 10, fill: '#878686', strokeColor: '#fff' },
//       batteryCap: { strokeColor: '#fff'},
//       batteryMeter: { fill: '#fff', lowBatteryValue: 21, outerGap: 2 },
//       readingText: {
//         lightContrastColor: '#111', darkContrastColor: '#111', lowBatteryColor: '#111',
//         fontSize: 30, y: '53%', showPercentage: false
//       }
//     }}/>
//   );
// };
