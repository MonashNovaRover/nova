import React from "react";
import { useSortable } from "@dnd-kit/sortable";
import { CSS } from "@dnd-kit/utilities";
import { Card } from "@nextui-org/react";

interface SortableItemProps {
  id: string;
  children: React.ReactNode;
}

export const SortablePoints: React.FC<SortableItemProps> = ({ id, children }) => {
  const { attributes, listeners, setNodeRef, transform, transition } =
    useSortable({ id });

  const style = {
    transform: CSS.Transform.toString(transform),
    transition,
  };

  return (
    <Card
      ref={setNodeRef}
      style={style}
      className="p-4 mb-4 rounded-lg  bg-gray-100 bg-neutral-800"
      {...attributes}
      {...listeners}
    >
      {children}
    </Card>
  );
};