import {Button, Table, TableBody, TableCell, TableColumn, TableHeader, TableRow, Tooltip} from "@nextui-org/react";
import {BooleanChip} from "../CameraComponent/components/BooleanChip.tsx";
import {ExternalLink, Pause, Play, Square} from "react-feather";
import {CircleFill} from "react-bootstrap-icons";
import {useOnlineCameraSerials, useStreamingBifrost} from "../hooks/cameraBifrostHooks.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import {useCallback} from "react";

interface CamerasTableProps {
  refreshAvailabilies: () => void
}

/**
 * Camera table describing all online camera serials and providing infromation and streaming actions
 * @param refreshAvailabilies
 * @constructor
 */
export const CamerasTable = ({refreshAvailabilies}: CamerasTableProps ) => {
  const [startStreaming, pauseStreaming, stopStreaming] = useStreamingBifrost(refreshAvailabilies);
  const onlineSerials = useOnlineCameraSerials();

  const cameraStreamerMap = useSelector(
    (state: RootState) => state.cameraStreamerState.cameras
  );

  const booleanChip = useCallback((serial: string) => <BooleanChip
    boolean={!!cameraStreamerMap[serial]}
    trueText="Streaming"
    falseText="Idle"
    falseColor="primary"
    variant="flat"
  />, [cameraStreamerMap])

  return (
    <Table
      removeWrapper
      isCompact
      className="hide-scrollbar w-full table-fixed"
      isHeaderSticky
    >
      <TableHeader>
        <TableColumn>Serial</TableColumn>
        <TableColumn>Status</TableColumn>
        <TableColumn align="end">
          <div className="flex flex-row justify-end">Actions</div>
        </TableColumn>
      </TableHeader>
      <TableBody
        emptyContent={"No Cameras Detected. Check if Cameras is running"}
      >
        {onlineSerials.map((serial) => (
          <TableRow>
            <TableCell className="flex flex-row gap-3 items-center">
              <CircleFill size={12} color="#17c964"/>
              <Tooltip content={serial} showArrow placement="top-start" color="default">
                <span className="truncate block">
                  {serial.length > 16 ? serial.slice(0, 16 - 1) + "…" : serial}
                </span>
              </Tooltip>
            </TableCell>
            <TableCell>
              {booleanChip(serial)}
            </TableCell>
            <TableCell>
              <div className="flex flex-row gap-2 justify-end">
                <Button
                  isIconOnly
                  size="sm"
                  color="primary"
                  onPressStart={() => startStreaming([serial], false)}
                >
                  <Play size="15px" fill="white" />
                </Button>
                <Button
                  isIconOnly
                  size="sm"
                  color="warning"
                  onPressStart={() => pauseStreaming([serial], false)}
                >
                  <Pause size="15px" fill="white" />
                </Button>
                <Button
                  isIconOnly
                  size="sm"
                  color="danger"
                  onPressStart={() => stopStreaming([serial], false)}
                >
                  <Square size="15px" fill="white" />
                </Button>
                <Button
                  isIconOnly
                  size="sm"
                  color="default"
                  onPress={() => window.open(
                    `/cameras/${serial}?autostart=true`,
                    "_blank",
                    "rel=noopener noreferrer"
                  )
                  }
                >
                  <ExternalLink size="15px" fill="white" />
                </Button>
              </div>
            </TableCell>
          </TableRow>
        ))}
      </TableBody>
    </Table>
  )
}