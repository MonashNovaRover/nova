import { Card, Button } from "@nextui-org/react";

export default function MySidebar() {
  return (
    <Card
      className="
        h-full
        rounded-none
        shadow-none
        border-none
        p-3
        flex flex-col gap-2
      "
    >
      <Button variant="light" fullWidth>
        Dashboard
      </Button>

      <Button variant="light" fullWidth>
        Sensors
      </Button>

      <Button variant="light" fullWidth>
        Controls
      </Button>
    </Card>
  );
}