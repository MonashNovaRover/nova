import {
  Button,
  Card,
  CardBody,
  CardHeader,
  Table,
  TableBody,
  TableCell,
  TableColumn,
  TableHeader,
  TableRow,
} from "@nextui-org/react";
import { useState } from "react";
import { ChevronDoubleDown, ChevronDoubleUp } from "react-bootstrap-icons";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState";
import { AnimatePresence, motion } from "framer-motion";
import { Trash } from "react-feather";
import { ToolTipButton } from "../../shared/TooltipButton";
import { useCartographerActions } from "../../../redux/actions/useCartographerActions";

export const BottomOverlay = () => {
  const [overlayOpen, setOverlayOpen] = useState(false);
  const points = useSelector(
    (state: RootState) => state.cartographerState.points
  );

  const { deletePoint } = useCartographerActions();

  return (
    <motion.div
      animate={{
        height: overlayOpen ? "400px" : "unset",
      }}
      transition={{ duration: 0.3 }}
    >
      <Card fullWidth className="h-full">
        <CardHeader className="w-full flex flex-row justify-between ">
          <div className="">Rover</div>
          <div className="">
            <Button
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
                </motion.div>
              </motion.div>
            </CardBody>
          )}
        </AnimatePresence>
      </Card>
    </motion.div>
  );
};
