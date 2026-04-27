import React, { useState } from "react";
import {
  DndContext,
  closestCenter,
  PointerSensor,
  useSensor,
  useSensors,
  DragEndEvent,
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
  Checkbox,
  Card,
  Divider,
} from "@nextui-org/react";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../../ros/services/rosService.ts";
import { GoalType, MapPoint } from "../../../../redux/models/CartographerState.ts";
import { IRosNovaInterfacesCartographerCommandRequest } from "../../../../ros/rosTypes.ts";
import { useSelector } from "react-redux";
import { RootState } from "../../../../redux/RootState.ts";
import { SortablePoints } from "../../../maps/Cartographer/components/SortablePoints.tsx";

export const CartographerGoalModal: React.FC<{
  isOpen: boolean;
  onClose: () => void;
}> = ({ isOpen, onClose }) => {
  const sensors = useSensors(useSensor(PointerSensor));
  
  const serviceBifrost = useBifrost({
    service: RosService.CARTOGRAPHER_COMMAND,
  });


  const { points } = useSelector(
    (state: RootState) => state.cartographerState
  );
  
  
  const [selection, setSelection] = useState<number[]>([]);

  const handleDragEnd = (event: DragEndEvent) => {
    const { active, over } = event;
    if (active.id !== over!.id) {
      const oldIndex = selection.findIndex((p) => points[p].name === active.id);
      const newIndex = selection.findIndex((p) => points[p].name === over!.id);
      setSelection(arrayMove(selection, oldIndex, newIndex));
    }
  };

  const toggleSelection = (name: string) => {
    const i = points.findIndex((v)=>v.name === name);
    if (selection.includes(i)) {
      setSelection((prev)=>prev.filter((v)=>v!==i))
    } else {
      setSelection((prev)=>[...prev, points.findIndex((v)=>v.name === name)])
    }
  };


  if (!isOpen && selection.length > 0) setSelection([]);


  const sendCartographerPoints = () => {
    const selected = selection.map((v)=>points[v]);
    const goals = selected.map((item) => ({
      latitude: item.lat,
      longitude: item.long,
    }));
    const type = selected.map((item) => item.labelNumber).pop();
    let search_radius = 0;
    if (type === GoalType.AR_TAG)
      search_radius = 5;
    else if (type === GoalType.OBJECT)
      search_radius = 10
    console.log("calling service with:", { goals: goals, type: type, search_radius: search_radius });
    serviceBifrost.callService({ goals: goals, type: type, search_radius: search_radius } as IRosNovaInterfacesCartographerCommandRequest);
  };

  const renderPoints = (points: MapPoint[], isSortable: boolean) => {
    return points.map((point) => (
      isSortable ? (
        <SortablePoints key={point.name} id={point.name}>
          {renderPointContent(point)}
        </SortablePoints>
      ) : (
        <Card
          key={point.name}
          className="p-4 mb-4 rounded-lg bg-gray-100 bg-neutral-800"
        >
          {renderPointContent(point)}
        </Card>
      )
    ));
  };

  const renderPointContent = (point: MapPoint) => (
    <div className="flex justify-between items-center">
      <Checkbox
        isSelected={selection.includes(points.indexOf(point))}
        onChange={() => toggleSelection(point.name)}
      >
        <span className="flex-shrink-0 font-bold">{point.name}</span>
      </Checkbox>
      <span className="flex-shrink-0">{point.lat}</span>
      <span className="flex-shrink-0">{point.long}</span>
      <div className="flex-shrink-0 flex justify-end w-24">
        {point.labelNumber !== null && GoalType[point.labelNumber] && (
          <Chip
            className={`ml-2 ${
              point.labelNumber == GoalType.GNSS
                ? "bg-blue-200 text-blue-800 dark:bg-blue-700 dark:text-blue-200"
                : point.labelNumber == GoalType.AR_TAG
                ? "bg-green-200 text-green-800 dark:bg-green-700 dark:text-green-200"
                : point.labelNumber == GoalType.OBJECT
                ? "bg-yellow-200 text-yellow-800 dark:bg-yellow-700 dark:text-yellow-200"
                : "bg-gray-200 text-gray-800 dark:bg-gray-700 dark:text-gray-200"
            }`}
            size="sm"
            radius="sm"
          >
            {point.labelName}
          </Chip>
        )}
      </div>
    </div>
  );

  const selectedItems = selection.map((v)=>points[v])
  const unselectedItems = points.filter((_, i) => !selection.includes(i));

  return (
    <Modal
      isOpen={isOpen}
      className="dark text-foreground"
      size="5xl"
      scrollBehavior="inside"
      onClose={onClose}
    >
      <ModalContent>
        <ModalHeader>Publish Goals</ModalHeader>
        <ModalBody>
          <div>
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
                  items={selectedItems.map((item) => item.name)}
                  strategy={verticalListSortingStrategy}
                >
                  <h3 className="my-4 font-bold">Selected Points</h3>
                  {renderPoints(selectedItems, true)}
                </SortableContext>
              </DndContext>
            </div>
            <Divider className="my-4" />
            <h3 className="font-bold my-4">Unselected Points</h3>
            {renderPoints(unselectedItems, false)}
          </div>
        </ModalBody>
        <ModalFooter>
          <Button onPress={sendCartographerPoints}>Publish</Button>
        </ModalFooter>
      </ModalContent>
    </Modal>
  );
};