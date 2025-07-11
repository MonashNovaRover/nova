import LSObjectCard from "./LSObjectCard.tsx";
import SiteSelectorTest from "./SiteSelectorTest.tsx";
import GenericStoreCard from "./GenericStoreCard.tsx";
import {Button, Card, CardBody} from "@nextui-org/react";

export default function TestStateView () {

  return (
    <div className="grid grid-rows-3 gap-4 py-4 px-4">
      <div className="grid grid-cols-4 gap-4">
        <GenericStoreCard/>
        <GenericStoreCard/>
        <SiteSelectorTest/>
        <SiteSelectorTest/>
      </div>
      <div className="grid grid-cols-2 gap-4">
        <LSObjectCard/>
        <LSObjectCard/>
      </div>
      <div className="grid grid-cols-2 gap-4">
        <Card className="grid grid-cols-1">
          <CardBody className="flex flex-row gap-4 m-8">
            <Button color="default">Default</Button>
            <Button color="primary">Primary</Button>
            <Button color="secondary">Secondary</Button>
            <Button color="success">Success</Button>
            <Button color="warning">Warning</Button>
            <Button color="danger">Danger</Button>
          </CardBody>
        </Card>
      </div>
    </div>
  );
}