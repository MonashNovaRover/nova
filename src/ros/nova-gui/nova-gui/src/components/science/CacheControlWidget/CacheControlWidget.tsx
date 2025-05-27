import React, {useEffect, useState} from "react";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import {Card, CardBody, CardHeader, CardProps, Tab, Tabs} from "@nextui-org/react";
import {IRosNovaInterfacesCacheCommandResponse} from "../../../ros/rosTypes.ts";

export interface CacheControlWidgetProps extends CardProps {
  label?: string
  service: RosService
}

const CacheControlWidget: React.FC<CacheControlWidgetProps> = (props) => {
  const [selected, setSelected] = useState<number>(2);
  const bifrost = useBifrost({service: props.service});

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const onSelectionChange = (key: number | string) => {
    bifrost.callService({angle: Number(key)}, {
      handleResponse: (response) => {
        const boolResponse = response as IRosNovaInterfacesCacheCommandResponse;
        if (boolResponse?.success) {
          setSelected(Number(key));
        }
      }
    });
  }

  return <Card {...props}>
    <CardHeader>{props.label ?? props.service}</CardHeader>
    <CardBody>
      <Tabs key="tabs" color="primary" radius="full" fullWidth selectedKey={selected.toString()} onSelectionChange={onSelectionChange}>
        <Tab key="0" title="0°" />
        <Tab key="1" title="90°" />
        <Tab key="2" title="180°" />
      </Tabs>
    </CardBody>
  </Card>
}

export default CacheControlWidget