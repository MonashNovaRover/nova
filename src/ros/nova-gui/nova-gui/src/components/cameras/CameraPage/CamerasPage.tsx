import {Button, Card, CardHeader, CardProps} from "@nextui-org/react";
import {Eye, EyeOff, Menu, Move} from "react-feather";
import { useState } from "react";
import {CameraControlPanelModal,} from "./CamerasControlPanelModal.tsx";
import SegmentedPicker from "../../../components/shared/components/SegmentedPicker/SegmentedPicker.tsx";
import { SaveAllCamerasModal } from "../../navbar/TopBar/SaveAllCamerasModal.tsx";
import { CameraPresetDropdown } from "../CameraPresetDropdown.tsx";
import { useSortable } from "@dnd-kit/react/sortable";
import { UniqueIdentifier } from "@dnd-kit/core";
import SerialMappedCameraComponent from "../../../views/shared/CamerasPage/SerialMappedCameraComponent.tsx";
import {CameraViewConfig} from "../../../views/shared/CamerasPage/CameraViewConstants.tsx";

export interface SortableProps extends CardProps {
  sortId: UniqueIdentifier,
  index: number,
}

/**
 * Creates a button on the top right that allows a camera component to be moved
 * @param props
 * @constructor
 */
const Sortable = (props: SortableProps) => {
  const {ref, handleRef} = useSortable({id: props.sortId, index: props.index});
  const [isHovered, setIsHovered] = useState(false);

  return (
    <Card
      ref={ref}
      onMouseEnter={()=>setIsHovered(true)}
      onMouseLeave={()=>setIsHovered(false)}
      {...props}
    >
      <CardHeader className="absolute z-1 top-0 right-0 justify-end">
        {isHovered &&
            <Button className="z-50" isIconOnly size="sm" ref={handleRef}>
                <Move size="15px"/>
            </Button>
        }
      </CardHeader>
      {props.children}
    </Card>
  )
}


export interface CameraPageProps {
  refreshAvailabilities: () => void
  gridSize: number
  toggleSidebar: () => void;
  views: CameraViewConfig[];
}

/**
 * Page of camera components
 * @param props
 * @constructor
 */
export const CamerasPage = ({refreshAvailabilities, gridSize, views, toggleSidebar }: CameraPageProps) => {
  const [controlPanelOpen, setControlPanelOpen] = useState(false);
  const [isSaveModalOpen, setIsSaveModalOpen] = useState(false);

  const closeControlPanel = () => setControlPanelOpen(false);

  const [allCamsOn, setAllCamsOn] = useState(false);

  const [selectedTab, setSelectedTab] = useState(0);

  return (
    <div className="p-3 h-full overflow-auto">
      <div className="flex flex-row justify-between items-center gap-32 pl-1 mb-3">
        <div className="flex flex-row gap-3 items-center">
          <Button isIconOnly onPressStart={toggleSidebar} variant="ghost" color="primary">
            <Menu/>
          </Button>
          {!allCamsOn ? (
            <Button
              size="md"
              color="primary"
              className="w-28"
              onPress={() => setAllCamsOn(true)}
            >
              <Eye size="15px" fill="white" /> Show All
            </Button>
          ) : (
            <Button
              size="md"
              color="danger"
              onPress={() => setAllCamsOn(false)}
            >
              <EyeOff size="15px" fill="white" /> Hide All
            </Button>
          )}
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

      <div className="grid gap-3 items-start" style={{ gridTemplateColumns: `repeat(${gridSize}, minmax(0, 1fr))` }}>
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
