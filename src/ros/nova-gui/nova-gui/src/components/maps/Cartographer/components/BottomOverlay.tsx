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
import CopyableInput from "../../../shared/components/CopyableInput/CopyableInput.tsx";
import { useState } from "react";
import { ChevronCompactDown, ChevronCompactUp, ChevronDoubleDown, ChevronDoubleUp } from "react-bootstrap-icons";
import { useSelector } from "react-redux";
import { RootState } from "../../../../redux/RootState.ts";
import { AnimatePresence, motion } from "framer-motion";
import { Navigation, Trash } from "react-feather";
import { ToolTipButton } from "../../../shared/components/TooltipButton.tsx";
import { useCartographerActions } from "../../../../redux/actions/useCartographerActions.ts";
import { MapTile } from "../config.tsx";
import { MapPoint } from "../../../../redux/models/CartographerState.ts";
import React from "react";

interface BottomOverlayProps {
  mapTile: MapTile;
  setMapTile: (tile: MapTile) => void;
  deletePoint: (point: MapPoint) => void;
  bottomOverlayComponents?: React.ReactNode[];
}

export const BottomOverlay : React.FC<BottomOverlayProps> = ({mapTile, setMapTile, deletePoint, bottomOverlayComponents = []}) => {
  const [overlayVisible, setOverlayVisible] = useState(false);
  const [overlayOpen, setOverlayOpen] = useState(false);
  const { points, centerOnRover, trackRover } = useSelector(
    (state: RootState) => state.cartographerState
  );
  const rover = useSelector((state: RootState) => state.roverLocationStore)
  const base = useSelector((state: RootState) => state.baseLocationStore)

  const { toggleRoverCentering, toggleRoverTracking } =
    useCartographerActions();
  
  const toggleOverlay = () => {
    setOverlayVisible(!overlayVisible);
    if (!overlayVisible) {
      setOverlayOpen(false);
    }
  };

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
      {overlayVisible && ( 
        <motion.div
          initial={{ height: 0 }}
          animate={{ height: overlayOpen ? 400 : "auto" }}
          transition={{ duration: 0.4, ease: "easeInOut" }}
          className="overflow-hidden"
      >
        <Card fullWidth className="h-full rounded-none" style={{ maxHeight: "400px" }}>
          <CardHeader className="w-full flex flex-row justify-between gap-3 items-center">
              <div className="flex flex-row gap-3">
              <CopyableInput
                readOnly
                value={String(base.latitude)}
                placeholder={`Base Latitude`}
                label="Base Latitude"/>
              <CopyableInput
                readOnly
                value={String(base.longitude)}
                placeholder={`Base Longitude`}
                label="Base Longitude"/>
              <CopyableInput
                readOnly
                value={String(rover.latitude)}
                placeholder={`Rover Latitude`}
                label="Rover Latitude"/>
              <CopyableInput
                readOnly
                value={String(rover.longitude)}
                placeholder={`Rover Longitude`}
                label="Rover Longitude"/>
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
              <Button
                variant="shadow"
                fullWidth
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
              <CardBody className="overflow-y-auto">
                <motion.div
                  className="w-full flex flex-row"
                  initial={{ opacity: 0, height: 0 }}
                  animate={{ opacity: 1, height: "auto" }}
                  exit={{ opacity: 0, height: 0 }}
                  transition={{
                    duration: 0.2,
                    ease: "easeInOut",
                  }}
                >
                  <div className="flex-1">
                    <Table removeWrapper title="Map Points" aria-label="Map Points">
                      <TableHeader>
                        <TableColumn>Name</TableColumn>
                        <TableColumn>Latitude</TableColumn>
                        <TableColumn>Longitude</TableColumn>
                        <TableColumn>Label</TableColumn>
                        <TableColumn align="end">
                          <div className="flex flex-row justify-end">Actions</div>
                        </TableColumn>
                      </TableHeader>
                      <TableBody emptyContent="Add Points on the Map to Display here">
                        {points.map((point) => (
                          <TableRow key={point.name}>
                            <TableCell>{point.name}</TableCell>
                            <TableCell>{point.lat}</TableCell>
                            <TableCell>{point.long}</TableCell>
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
                        ))}
                      </TableBody>
                    </Table>
                  </div>
                </motion.div>
              </CardBody>
            )}
          </AnimatePresence>
        </Card>
      </motion.div>
      )}
    </div>
  );
};
