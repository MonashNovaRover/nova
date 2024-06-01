import {
  Button,
  Card,
  CardBody,
  CardHeader,
  Select,
  SelectItem,
  Table,
  TableBody,
  TableCell,
  TableColumn,
  TableHeader,
  TableRow,
} from "@nextui-org/react";
import CopyableInput from "../../CopyableInput/CopyableInput.tsx";
import { useState } from "react";
import { ChevronDoubleDown, ChevronDoubleUp } from "react-bootstrap-icons";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState";
import { AnimatePresence, motion } from "framer-motion";
import { Navigation, Trash } from "react-feather";
import { ToolTipButton } from "../../shared/TooltipButton";
import { useCartographerActions } from "../../../redux/actions/useCartographerActions";
import { MapTile } from "../config.tsx";

interface BottomOverlayProps {
  mapTile: MapTile;
  setMapTile: (tile: MapTile) => void;
}

export const BottomOverlay : React.FC<BottomOverlayProps> = ({mapTile, setMapTile}) => {
  const [overlayOpen, setOverlayOpen] = useState(false);
  const { points, centerOnRover, trackRover } = useSelector(
    (state: RootState) => state.cartographerState
  );

  const { deletePoint, toggleRoverCentering, toggleRoverTracking } =
    useCartographerActions();

  return (
    <motion.div
      animate={{
        height: overlayOpen ? "400px" : "unset",
      }}
      transition={{ duration: 0.3 }}
    >
      <Card fullWidth className="h-full">
        <CardHeader className="w-full flex flex-row justify-between ">
          <div className=""></div>
          <div className="flex flex-row align-middle gap-2">
          <Select 
            selectedKeys={[mapTile]}
            label="Map Tiles"
            placeholder="Select Tiles"
            onChange={(e) => setMapTile(e.target.value as MapTile)}
            >
            {Object.values(MapTile).map((tile) => (
              <SelectItem 
                key={tile} >
                {tile}
              </SelectItem>
            ))}
            </Select>
            <Button
              variant="shadow"
              color={trackRover ? "primary" : "default"}
              onClick={toggleRoverTracking}
            >
              Track Rovey
            </Button>
            <ToolTipButton
              tooltipContent="Center Rover"
              isIconOnly
              variant="shadow"
              color={centerOnRover ? "primary" : "default"}
              onClick={toggleRoverCentering}
            >
              <Navigation className="w-5" />
            </ToolTipButton>
            <Button
              variant="shadow"
              isIconOnly
              fullWidth
              onClick={() => setOverlayOpen(!overlayOpen)}
            >
              {overlayOpen ? <ChevronDoubleDown /> : <ChevronDoubleUp />}
            </Button>
          </div>
        </CardHeader>
        <AnimatePresence>
          {overlayOpen && (
            <CardBody>
              <motion.div
                className="w-full flex flex-row"
                initial={{ opacity: 0 }}
                animate={{ opacity: 1 }}
                exit={{ opacity: 0 }}
              >
                <motion.div className="flex-1">
                  <Table removeWrapper title="Map Points">
                    <TableHeader>
                      <TableColumn>Name</TableColumn>
                      <TableColumn>Latitude</TableColumn>
                      <TableColumn>Longitude</TableColumn>
                      <TableColumn align="end">
                        <div className="flex flex-row justify-end">Actions</div>
                      </TableColumn>
                    </TableHeader>
                    <TableBody emptyContent="Add Points on the Map to Display here">
                      {points.map((point) => (
                        <TableRow>
                          <TableCell>{point.name}</TableCell>
                          <TableCell>{point.lat}</TableCell>
                          <TableCell>{point.long}</TableCell>
                          <TableCell className="flex flex-row justify-end">
                            <ToolTipButton
                              isIconOnly
                              size="sm"
                              tooltipContent="Delete"
                              onClick={() => deletePoint(point)}
                            >
                              <Trash className="w-4" />
                            </ToolTipButton>
                          </TableCell>
                        </TableRow>
                      ))}
                    </TableBody>
                  </Table>
                  <CopyableInput className="grow basis-1"
                     size="md" labelPlacement="outside"
                     label={"ROS2 Action Goal Poses"}
                     //value={`${points.map(point => `{latitude: ${point.lat.toString()}, longitude: ${point.long.toString()}}`)}`}  
                     value={`./bt-navigator/bin/ros2 action send_goal /urc_navigator nova_auto_interfaces/action/NavigateURC "{gps_poses: [{${points.map(point => `position: {latitude: ${point.lat.toString()}, longitude: ${point.long.toString()}}`)}}], behavior_tree: '$HOME/src/ros/rover/nav2_autonomous/nova_behavior_tree/behavior_tree/urc/urc_through_poses_search.xml'}" `}
                     copyValue={`./bt-navigator/bin/ros2 action send_goal /urc_navigator nova_auto_interfaces/action/NavigateURC "{gps_poses: [{${points.map(point => `position: {latitude: ${point.lat.toString()}, longitude: ${point.long.toString()}}`)}}], behavior_tree: '$HOME/src/ros/rover/nav2_autonomous/nova_behavior_tree/behavior_tree/urc/urc_through_poses_search.xml'}"`}
                     classNames={{
                       input: "font-mono",
                       inputWrapper: "data-[hover=true]:bg-default-100"
                     }}
                     placeholder={"##.#### %"}>
                  </CopyableInput>

                </motion.div>
              </motion.div>
            </CardBody>
          )}
        </AnimatePresence>
      </Card>
    </motion.div>
  );
};
