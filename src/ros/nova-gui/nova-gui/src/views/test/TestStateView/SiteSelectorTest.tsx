import SiteSelectWidget from "./SiteSelectWidget.tsx";
import {useState} from "react";
import {Card, CardBody} from "@nextui-org/react";


export default function SiteSelectorTest() {


  const [site, setSite] = useState("site1");


  return (
    <Card>
      <CardBody>
        <SiteSelectWidget
          onValueChanged={setSite}
          hideCard={true}
          hideSiteType={false}
        />
      </CardBody>
      <CardBody className="text-center">
        {site}
      </CardBody>
    </Card>
  )
}


