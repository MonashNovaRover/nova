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
  Modal,
  ModalContent,
  ModalFooter,
  ModalHeader
} from "@nextui-org/react";
import CopyableInput from "../../../shared/components/CopyableInput/CopyableInput.tsx";
import { useState } from "react";
import { ChevronCompactDown, ChevronCompactUp, ChevronDoubleDown, ChevronDoubleUp } from "react-bootstrap-icons";
import { useSelector } from "react-redux";
import { RootState } from "../../../../redux/RootState.ts";
import { AnimatePresence, motion } from "framer-motion";
import { Navigation, Trash, Truck, Twitter, X, Target } from "react-feather";
import { ToolTipButton } from "../../../shared/components/TooltipButton.tsx";
import { useCartographerActions } from "../../../../redux/actions/useCartographerActions.ts";
import { MapTile } from "../config.tsx";
import { MapPoint, Vehicle } from "../../../../redux/models/CartographerState.ts";
import React from "react";
import {displayMapCoordinate, DisplayMapCoordinate, useDisplayMapCoordinate} from "../utils/convertCoords.ts";
import { useGenericStore } from "../../../../hooks/useGenericStore.ts";

interface BottomOverlayProps {
  mapTile: MapTile;
  setMapTile: (tile: MapTile) => void;
  deletePoint: (point: MapPoint) => void;
  bottomOverlayComponents?: React.ReactNode[];
  enableDroneTracking?: boolean;
}

export const BottomOverlay : React.FC<BottomOverlayProps> = ({mapTile, setMapTile, deletePoint, bottomOverlayComponents = [], enableDroneTracking = false}) => {
  const [cartographerCoordinateFormat] = useGenericStore<number>("cartographerCoordinateFormat");
  const [overlayVisible, setOverlayVisible] = useState(true);
  const [overlayOpen, setOverlayOpen] = useState(false);
  const { points, centerOnRover, trackRover, showTrackRover, centerOnDrone, trackDrone, showTrackDrone, focusVehicle } = useSelector(
    (state: RootState) => state.cartographerState
  );
  const rover = useSelector((state: RootState) => state.roverLocationStore)
  const base = useSelector((state: RootState) => state.baseLocationStore)
  const drone = useSelector((state: RootState) => state.droneLocationStore)

  const { toggleRoverCentering, toggleRoverTracking, toggleShowRoverTracking, toggleDroneCentering, toggleDroneTracking, toggleShowDroneTracking, handleFocusVehicle, toggleShowSearchZones } =
    useCartographerActions();

  const vehicles = {
    [Vehicle.ROVER]: {
      label: "Rover",
      location: rover,
      track: trackRover,
      showTrack: showTrackRover,
      centerOn: centerOnRover,
      toggleTracking: toggleRoverTracking,
      toggleShowTracking: toggleShowRoverTracking,
      toggleCentering: toggleRoverCentering,
    },
    ...(enableDroneTracking && {
      [Vehicle.DRONE]: {
        label: "Drone",
        location: drone,
        track: trackDrone,
        showTrack: showTrackDrone,
        centerOn: centerOnDrone,
        toggleTracking: toggleDroneTracking,
        toggleShowTracking: toggleShowDroneTracking,
        toggleCentering: toggleDroneCentering,
      },
    }),
  };

  const currentVehicle =
    vehicles[focusVehicle] ?? vehicles[Vehicle.ROVER];

  const [showModal, setShowModal] = useState(false);

  const modal = (
    <Modal
      size="xs"
      className="dark text-foreground"
      isOpen={showModal}
      onClose={() => setShowModal(false)}
    >
      <ModalContent>
        <ModalHeader className="flex flex-col gap-1">
          Delete {currentVehicle.label} trace?
        </ModalHeader>
        <ModalFooter>
          <Button variant="light" onPressStart={() => setShowModal(false)}>
            Close
          </Button>
          <Button
            color="danger"
            onPressStart={() => {
              currentVehicle.toggleTracking();
              setTimeout(() => currentVehicle.toggleTracking(), 0);
              setShowModal(false);
            }}
          >
            Delete
          </Button>
        </ModalFooter>
      </ModalContent>
    </Modal>
  )
  
  const toggleOverlay = () => {
    setOverlayVisible(!overlayVisible);
    if (!overlayVisible) {
      setOverlayOpen(false);
    }
  };

  const {lat: baseLat, long: baseLong} = useDisplayMapCoordinate({lat: base.latitude, long: base.longitude})
  const {lat: vehLat, long: vehLong} = useDisplayMapCoordinate({lat: currentVehicle.location.latitude, long: currentVehicle.location.longitude})

  return (
    <div className="relative w-full">
      <div className="flex justify-end p-2 z-50">
      <Button
          variant="shadow"
          isIconOnly
          onClick={toggleOverlay}
          className="absolute -top-4 shadow-md w-20 h-8 flex items-center justify-center"
          style={{
            borderBottomLeftRadius: "0px",
            borderBottomRightRadius: "0px",
            borderTopLeftRadius: "8px",
            borderTopRightRadius: "8px",
          }}
        >
          {overlayVisible ? <ChevronCompactDown /> : <ChevronCompactUp />}
        </Button>
      </div>
      <AnimatePresence>
      {overlayVisible && ( 
        <motion.div
          initial={{ height: 0 }}
          animate={{ height: overlayOpen ? 400 : "auto" }}
          exit={{ height: 0, opacity: 1 }}
          transition={{ duration: 0.4, ease: "easeInOut" }}
          className="overflow-hidden"
        >
          <Card fullWidth className="h-full rounded-none" style={{ maxHeight: "400px" }}>
            <CardHeader className="w-full flex flex-row justify-between gap-3 items-center" style={{ height: "80px" }}>
                <div className="flex flex-row gap-3">
                <CopyableInput
                  readOnly
                  value={baseLat}
                  placeholder={`Base Latitude`}
                  label="Base Latitude"/>
                <CopyableInput
                  readOnly
                  value={baseLong}
                  placeholder={`Base Longitude`}
                  label="Base Longitude"/>
                <CopyableInput
                  readOnly
                  value={vehLat}
                  placeholder={`${currentVehicle.label} Latitude`}
                  label={`${currentVehicle.label} Latitude`}/>
                <CopyableInput
                  readOnly
                  value={vehLong}
                  placeholder={`${currentVehicle.label} Longitude`}
                  label={`${currentVehicle.label} Longitude`}/>
                  <CopyableInput
                  readOnly
                  value={String(currentVehicle.location.altitude)}
                  placeholder={`${currentVehicle.label} Altitude`}
                  label={`${currentVehicle.label} Altitude`}/>
                  <CopyableInput
                  readOnly
                  value={String(currentVehicle.location.heading)}
                  placeholder={`${currentVehicle.label} Heading`}
                  label={`${currentVehicle.label} Heading`}/>
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
                </div>
                <div className="flex flex-row gap-3 items-center">
                {bottomOverlayComponents.map((component, index) => (
                  <React.Fragment key={index}>
                    {component}
                  </React.Fragment>
                ))}
                <div className="relative">
                  <Button
                    variant="shadow"
                    fullWidth
                    color={currentVehicle.showTrack ? "primary" : "default"}
                    onClick={currentVehicle.toggleShowTracking}
                  >
                    {focusVehicle == Vehicle.ROVER ? "Track Rovey" : "Track Droney"}
                  </Button>
                  <button
                    className="absolute -top-2 -right-2 w-5 h-5 rounded-full bg-[#848482] hover:bg-[#6b6b69] active:bg-[#4a4a48] flex items-center justify-center transition-colors"
                    onClick={() => {
                      setShowModal(true);
                    }}
                  >
                    <X className="w-3 h-3 text-white" />
                  </button>
                  {modal}
                </div>
                  <ToolTipButton
                    tooltipContent={`Toggle Show Search Zone`}
                    isIconOnly
                    variant="shadow"
                    color={currentVehicle.centerOn ? "primary" : "default"}
                    onPressStart={toggleShowSearchZones}
                  >
                    <Target className="w-5" />
                  </ToolTipButton>
                <ToolTipButton
                  tooltipContent={`Center ${currentVehicle.label}`}
                  isIconOnly
                  variant="shadow"
                  color={currentVehicle.centerOn ? "primary" : "default"}
                  onClick={currentVehicle.toggleCentering}
                >
                  <Navigation className="w-5" />
                </ToolTipButton>
                {enableDroneTracking && (
                  <ToolTipButton
                    tooltipContent={focusVehicle == Vehicle.ROVER ? "Focus Drone" : "Focus Rover"}
                    isIconOnly
                    variant="shadow"
                    onClick={handleFocusVehicle}
                  >
                    {focusVehicle == Vehicle.ROVER ? <Twitter /> : <Truck />}
                  </ToolTipButton>
                )}
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
                <motion.div
                  className="w-full flex flex-row overflow-hidden"
                  initial={{ opacity: 1, height: 0 }}
                  animate={{ opacity: 1, height: "320px" }}
                  exit={{ opacity: 1, height: 0 }}
                  transition={{ duration: 0.4, ease: "easeInOut" }}
                >
                  <CardBody className="overflow-y-auto" style={{ height: "320px" }}>
                    <div className="flex-1">
                      <Table removeWrapper title="Map Points" aria-label="Map Points" >
                        <TableHeader>
                          <TableColumn>Name</TableColumn>
                          <TableColumn>Latitude</TableColumn>
                          <TableColumn>Longitude</TableColumn>
                          <TableColumn>Radius</TableColumn>
                          <TableColumn>Label</TableColumn>
                          <TableColumn align="end">
                            <div className="flex flex-row justify-end">Actions</div>
                          </TableColumn>
                        </TableHeader>
                        <TableBody emptyContent="Add Points on the Map to Display here">
                          {points.map((point) => {
                            const {lat: pointLat, long: pointLong} = displayMapCoordinate({lat: point.lat, long: point.long}, cartographerCoordinateFormat)
                            return (
                            <TableRow key={point.name}>
                              <TableCell>{point.name}</TableCell>
                              <TableCell>{pointLat}</TableCell>
                              <TableCell>{pointLong}</TableCell>
                              <TableCell>{point.searchRadius != null ? `${point.searchRadius}m` : ''}</TableCell>
                              <TableCell>{point.labelName != null ? point.labelName : ''}</TableCell>
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
                          )})}
                        </TableBody>
                      </Table>
                    </div>
                  </CardBody>
                </motion.div>
              )}
            </AnimatePresence>
        </Card>
      </motion.div>
      )}
      </AnimatePresence>
    </div>
  );
};
