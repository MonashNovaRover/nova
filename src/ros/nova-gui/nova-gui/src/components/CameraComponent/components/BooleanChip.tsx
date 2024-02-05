import { Chip } from "@nextui-org/react";

type Color =
  | "success"
  | "default"
  | "primary"
  | "secondary"
  | "warning"
  | "danger";

interface BooleanChip {
  variant?: "dot" | "flat";
  boolean: boolean;
  trueText: string;
  falseText: string;
  trueColor?: Color;
  falseColor?: Color;
  size?: "sm" | "md" | "lg";
}

export const BooleanChip = (props: BooleanChip) => {
  const {
    boolean,
    falseText,
    trueText,
    variant,
    size = "sm",
    trueColor = "success",
    falseColor = "danger",
  } = props;

  return (
    <Chip
      variant={variant}
      size={size}
      className="rounded-md"
      color={boolean ? trueColor : falseColor}
    >
      {boolean ? trueText : falseText}
    </Chip>
  );
};
