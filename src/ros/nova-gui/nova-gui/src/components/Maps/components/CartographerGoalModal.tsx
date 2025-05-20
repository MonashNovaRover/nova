import React, { useEffect, useState } from "react";
import {
  DndContext,
  closestCenter,
  PointerSensor,
  useSensor,
  useSensors,
} from "@dnd-kit/core";
import {
  arrayMove,
  SortableContext,
  verticalListSortingStrategy,
} from "@dnd-kit/sortable";
import {
  restrictToVerticalAxis,
  restrictToWindowEdges,
  restrictToFirstScrollableAncestor,
} from "@dnd-kit/modifiers";
import {
  Modal,
  Button,
  ModalHeader,
  ModalFooter,
  ModalBody,
  ModalContent,
  Chip,
} from "@nextui-org/react";
import { SortablePoints } from "./SortablePoints";
import { ToolTipButton } from "../../shared/TooltipButton";
import { Trash } from "react-feather";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction";
import { RosService } from "../../../ros/services/rosService";
import { MapPoint } from "../../../redux/models/CartographerState";

export const CartographerGoalModal: React.FC<{
  isOpen: boolean;
  onClose: () => void;
  points: MapPoint[];
}> = ({ isOpen, onClose, points }) => {
  const [items, setItems] = useState(points);

  const serviceBifrost = useBifrost({
    service: RosService.CARTOGRAPHER_COMMAND,
  });

  const sendCartographerPoints = () => {
    const poses = items.map((item) => ({
      lat: item.lat,
      long: item.long,
    }));
    const types = items.map((item) => item.goalType);
    serviceBifrost.callServiceToRedux({ poses, types });
  };

  const sensors = useSensors(useSensor(PointerSensor));

  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  const handleDragEnd = (event: any) => {
    const { active, over } = event;
    if (active.id !== over.id) {
      const oldIndex = items.findIndex((item) => item.name === active.id);
      const newIndex = items.findIndex((item) => item.name === over.id);
      setItems(arrayMove(items, oldIndex, newIndex));
    }
  };

  useEffect(() => {
    setItems(points);
  }, [isOpen, points]);

  return (
    <Modal
      isOpen={isOpen}
      className="dark text-foreground"
      size="4xl"
      scrollBehavior="inside"
      onClose={onClose}
    >
      <ModalContent>
        <ModalHeader>Publish Goals</ModalHeader>
        <ModalBody>
          <div>
            <DndContext
              sensors={sensors}
              collisionDetection={closestCenter}
              modifiers={[
                restrictToVerticalAxis,
                restrictToWindowEdges,
                restrictToFirstScrollableAncestor,
              ]}
              onDragEnd={handleDragEnd}
            >
              <SortableContext
                items={items.map((item) => item.name)}
                strategy={verticalListSortingStrategy}
              >
                {items.map((point) => {
                  return (
                    <SortablePoints key={point.name} id={point.name}>
                      <div className="flex justify-between items-center">
                        <span className="flex-shrink-0 w-1/4 font-bold">
                          {point.name}
                        </span>
                        <span className="flex-shrink-0 w-1/4">{point.lat}</span>
                        <span className="flex-shrink-0 w-1/4">
                          {point.long}
                        </span>
                        <Chip
                          className="ml-2 bg-gray-200 text-gray-800 dark:bg-gray-700 dark:text-gray-200"
                          size="sm"
                        >
                          {point.goalType}
                        </Chip>
                        <ToolTipButton
                          isIconOnly
                          size="sm"
                          tooltipContent="Remove"
                          onClick={() =>
                            setItems(
                              items.filter((item) => item.name !== point.name)
                            )
                          }
                        >
                          <Trash className="w-4" />
                        </ToolTipButton>
                      </div>
                    </SortablePoints>
                  );
                })}
              </SortableContext>
            </DndContext>
          </div>
        </ModalBody>
        <ModalFooter>
          <Button onPress={sendCartographerPoints}>Publish</Button>
        </ModalFooter>
      </ModalContent>
    </Modal>
  );
};
