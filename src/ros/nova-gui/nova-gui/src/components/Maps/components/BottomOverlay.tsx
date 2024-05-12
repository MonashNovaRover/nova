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
import { motion } from "framer-motion";

export const BottomOverlay = () => {
  const [overlayOpen, setOverlayOpen] = useState(false);
  return (
    <motion.div
      animate={{
        height: overlayOpen ? "400px" : "unset",
      }}
    >
      <Card fullWidth className="h-full">
        <CardHeader className="w-full flex flex-row justify-end ">
          <Button
            isIconOnly
            fullWidth
            onClick={() => setOverlayOpen(!overlayOpen)}
          >
            {overlayOpen ? <ChevronDoubleDown /> : <ChevronDoubleUp />}
          </Button>
        </CardHeader>
        {overlayOpen && (
          <CardBody>
            <div className="w-full flex flex-row">
              <div className="flex-1">
                <Table>
                  <TableHeader>
                    <TableColumn>Name</TableColumn>
                    <TableColumn>Latitude</TableColumn>
                    <TableColumn>Longitude</TableColumn>
                    <TableColumn align="end">
                      <div className="flex flex-row justify-end">Actions</div>
                    </TableColumn>
                  </TableHeader>
                  <TableBody>
                    <TableRow>
                      <TableCell>Point 1</TableCell>
                      <TableCell>Point 2</TableCell>
                      <TableCell>Point 3</TableCell>
                      <TableCell>Point 4</TableCell>
                    </TableRow>
                  </TableBody>
                </Table>
              </div>
              <div className="flex-1"></div>
            </div>
          </CardBody>
        )}
      </Card>
    </motion.div>
  );
};
