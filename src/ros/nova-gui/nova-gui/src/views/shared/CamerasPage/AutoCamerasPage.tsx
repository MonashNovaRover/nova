import { Button, Card, CardProps, CardHeader, Input } from "@nextui-org/react";
import { Play, Square } from "react-feather";
import { useCameraStreamer } from "../../../components/cameras/CameraComponent/hooks/useCameraStreamer";
import { CameraView } from "./CameraPageConstants";
import { useState } from "react";
import { CameraControlPanelModal } from "../../../components/cameras/CameraComponent/components/CamerasControlPanelModal";
import SerialMappedCameraComponent from "./SerialMappedCameraComponent.tsx";
import SegmentedPicker from "../../../components/shared/components/SegmentedPicker/SegmentedPicker.tsx";
import { SaveAllCamerasModal } from "../../../components/navbar/TopBar/SaveAllCamerasModal";
import { CameraPresetDropdown } from "../../../components/cameras/CameraPresetDropdown";
import { useSortable } from "@dnd-kit/react/sortable";
import { UniqueIdentifier } from "@dnd-kit/core";
import {Table as TableIcon} from "react-feather";

export interface SortableProps extends CardProps {
  sortId: UniqueIdentifier,
  index: number,
}

const Sortable = (props: SortableProps) => {
  const {ref, handleRef} = useSortable({id: props.sortId, index: props.index});
  const [isHovered, setIsHovered] = useState(false);

  return <Card ref={ref} 
    onMouseEnter={()=>setIsHovered(true)} 
    onMouseLeave={()=>setIsHovered(false)}
    {...props}>
      <CardHeader className="absolute z-1 top-0">
        {isHovered &&
          <Button className="z-50" isIconOnly size="sm" ref={handleRef}>
            <TableIcon size="15px"/>
          </Button>
        }
      </CardHeader>
      {props.children}
    </Card>
}

/**
 * TODO: remove
 *
 * This is a temp copy of CamerasPage.tsx meant only for the 2026 ARCh autonomous task,
 * the only change is having 3 instead of 4 columns of cameras.
 */

export interface CameraPageProps {
  views: CameraView[];
}

export const CameraPage = (props: CameraPageProps) => {
  const { views } = props;
  const [controlPanelOpen, setControlPanelOpen] = useState(false);
  const [isSaveModalOpen, setIsSaveModalOpen] = useState(false);

  const closeControlPanel = () => setControlPanelOpen(false);

  const { refreshAvailabilities } = useCameraStreamer();

  const [allCamsOn, setAllCamsOn] = useState(false);

  const [selectedTab, setSelectedTab] = useState(0);

  const [gridSize, setGridSize] = useState(3);

  return (
    <div className="p-3 flex flex-col gap-0">
      <div className="flex flex-row justify-between items-center gap-32 pl-1 mb-3">
        <div className="flex flex-row gap-3 items-center w-50">
          {!allCamsOn ? (
            <Button
              size="md"
              color="primary"
              className="w-28"
              onPress={() => setAllCamsOn(true)}
            >
              <Play size="15px" fill="white" /> Start All
            </Button>
          ) : (
            <Button
              size="md"
              color="danger"
              className="w-28"
              onPress={() => setAllCamsOn(false)}
            >
              <Square size="15px" fill="white" /> Stop All
            </Button>
          )}
          <div className="flex flex-row w-14 h-10 gap-2">
              <Input min={1} type="number" defaultValue="3" onChange={(e)=>setGridSize(+e.target.value)}/>
              <div className="absolute pointer-events-none">
                <span className="relative text-[0.7rem] inset-x-[0.3rem] inset-y-[-0.5rem]"># of Cols:</span>
              </div>
          </div>
        </div>
        <SegmentedPicker
          selectedIndex={selectedTab}
          onIndexChange={setSelectedTab}
          children={[
            views.map(v => v.viewTitle)
          ]}
          color="primary"
          className="pb-0"
          fullWidth
          variant="bordered"
        />

        <div className="flex flex-row gap-3">
          <Button
            size="md"
            color="primary"
            variant="ghost"
            className="w-36"
            onPress={() => setControlPanelOpen(true)}
          >
            Control Panel
          </Button>
          <CameraPresetDropdown onSavePress={() => setIsSaveModalOpen(true)} />
        </div>
      </div>

      {
        <div className={`grid grid-cols-${gridSize} gap-3`}>
          {[...new Set(views.flatMap((el)=>el.cameraSerials))].map((serial, i) => (
            <Sortable 
              key={"sort"+i} sortId={i} index={i} className={
              views[selectedTab].cameraSerials.includes(serial) ? "" : "hidden"
            }>
              <SerialMappedCameraComponent
                cameraSerial={serial}
                key={i}
                autostart={allCamsOn}
              />
            </Sortable>
          ))}
        </div>
      }

      <CameraControlPanelModal
        showModal={controlPanelOpen}
        closeModal={closeControlPanel}
        refreshAvailabilies={refreshAvailabilities}
      />
      <SaveAllCamerasModal
        isOpen={isSaveModalOpen}
        onClose={() => setIsSaveModalOpen(false)}
      />
    </div>
  );
};
