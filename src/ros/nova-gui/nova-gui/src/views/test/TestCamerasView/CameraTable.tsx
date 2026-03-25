import {Button, Table, TableBody, TableCell, TableColumn, TableHeader, TableRow} from "@nextui-org/react";
import {BooleanChip} from "../../../components/cameras/CameraComponent/components/BooleanChip.tsx";
import {Circle, ExternalLink, Pause, Play, Square} from "react-feather";
import {CircleFill} from "react-bootstrap-icons";

export const TestCameraTable = () => {
  // const cameras = ["mast_forward", "Surface_Surface_Camera_Front_200901010001"]
  const cameras = ["mast_forward", "mast_down"]

  return (
  <Table
    removeWrapper
    isCompact
    className="h-[72vh] hide-scrollbar w-full table-fixed"
    isHeaderSticky
  >
    <TableHeader>
      <TableColumn>Serial</TableColumn>
      {/*<TableColumn>Connection</TableColumn>*/}
      <TableColumn>Status</TableColumn>
      <TableColumn align="end">
        <div className="flex flex-row justify-end">Actions</div>
      </TableColumn>
    </TableHeader>
    <TableBody
      emptyContent={"No Cameras Detected. Check if Cameras is running"}
    >
      {cameras.map((serial) => (
        <TableRow>
          <TableCell className="flex flex-row gap-3 items-center">
            <CircleFill size={12} color="#17c964"/>
            <span className="truncate block">{serial}</span>
          </TableCell>
          {/*<TableCell>*/}
          {/*  <BooleanChip*/}
          {/*    boolean={true}*/}
          {/*    trueText="Online"*/}
          {/*    falseText="Offline"*/}
          {/*    variant="dot"*/}
          {/*  />*/}
          {/*</TableCell>*/}
          <TableCell>
            <BooleanChip
              boolean={serial === "mast_forward"}
              trueText="Streaming"
              falseText="Idle"
              falseColor="primary"
              variant="flat"
            />
          </TableCell>
          <TableCell>
            <div className="flex flex-row gap-2 justify-end">
              <Button
                isIconOnly
                size="sm"
                color="primary"
              >
                <Play size="15px" fill="white" />
              </Button>
              <Button
                isIconOnly
                size="sm"
                color="warning"
              >
                <Pause size="15px" fill="white" />
              </Button>
              <Button
                isIconOnly
                size="sm"
                color="danger"
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
)}