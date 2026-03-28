import {Renderable, Toast, ToastOptions, ValueOrFunction} from "react-hot-toast";
import {Card, CardBody, CardProps} from "@nextui-org/react";
import csgo from '../../../assets/meme/csgo.webm';
import willInitial from '../../../assets/meme/will-1.jpg';
import willEscaping from '../../../assets/meme/will-2.jpg';
import cartoonRobbers from '../../../assets/meme/cartoon_robbers.mp4';
import willJumpScare from '../../../assets/meme/will-y-jumpscare.jpg';


export type Message = ValueOrFunction<Renderable, Toast>

export interface inspirationalToastProps extends CardProps {};

export interface inspirationalToast {
  message: Message,
  timeout: number,
  options: ToastOptions,
  repeat?: boolean,
}

const defaultInspirationalToastOptions: ToastOptions = {
  duration: 5000,
  position: 'bottom-center',
}

const mediaHeight = 150;
const mediaWidth = 150;

const DefaultInspirationMessage: React.FC = (props: inspirationalToastProps) => (
  <Card>
    <CardBody {...props} className="font-mono text-xl">
      {props.children}
    </CardBody>
  </Card>);

export const inspirationalMessages: inspirationalToast[] = [
  {
    message: <DefaultInspirationMessage>
      wait sorry, i think i left something just outside the base station <br/>
      could you just quickly go and grab it for me?
    </DefaultInspirationMessage>,
    timeout: 10000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>the software subteam is soooooo cool, actually why don't you join us?</DefaultInspirationMessage>,
    timeout: 10000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>i think i'm actually losing it</DefaultInspirationMessage>,
    timeout: 10000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>meme gui isn't real, it can't hurt you</DefaultInspirationMessage>,
    timeout: 10000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>meme gui isn't just about ruining auto's day, it's also about learning React</DefaultInspirationMessage>,
    timeout: 10000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>67 67 67 67 67 67 67 67 67</DefaultInspirationMessage>,
    timeout: 10000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>can you do a little dance for me?</DefaultInspirationMessage>,
    timeout: 2000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>come on, just full send it into the wall</DefaultInspirationMessage>,
    timeout: 2000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>I can't wait to see Victor in a hairless state</DefaultInspirationMessage>,
    timeout: 2000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>auto stack is to software as the leaning tower of pisa is to buildings</DefaultInspirationMessage>,
    timeout: 2000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>wait, is it starting to rain?</DefaultInspirationMessage>,
    timeout: 2000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>i would sell my soul for 5 points</DefaultInspirationMessage>,
    timeout: 2000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>run "kfc" in terminal to get free food</DefaultInspirationMessage>,
    timeout: 2000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>come to software, we have free candy</DefaultInspirationMessage>,
    timeout: 2000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>a horse sized horse is indeed horse sized</DefaultInspirationMessage>,
    timeout: 2000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>yellow cubes aren't real, they can't hurt you</DefaultInspirationMessage>,
    timeout: 2000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>
      "Yeah f*** it I'm just gonna commit this to master untested" <br/>
      - Terry Tian (2026 Auto Team Lead)
    </DefaultInspirationMessage>,
    timeout: 10000,
    options: defaultInspirationalToastOptions,
  },
  {
    message: <DefaultInspirationMessage>
      <video autoPlay height={mediaHeight * 1.5} width={mediaWidth * 1.5}>
        <source src={csgo} type="video/webm"/>
      </video>
    </DefaultInspirationMessage>,
    timeout: 10000,
    options: {...defaultInspirationalToastOptions,
    duration: 10000},
  },
  {
    message: <DefaultInspirationMessage>
      <video autoPlay height={mediaHeight * 1.5} width={mediaWidth * 1.5}>
        <source src={cartoonRobbers} type="video/mp4"/>
      </video>
    </DefaultInspirationMessage>,
    timeout: 10000,
    options: {...defaultInspirationalToastOptions,
      duration: 5000},
  },
  {
    message: <DefaultInspirationMessage>
      <img src={willInitial} height={mediaHeight} width={mediaWidth}/>
    </DefaultInspirationMessage>,
    timeout: 2000,
    options: {...defaultInspirationalToastOptions},
  },
  {
    message: <DefaultInspirationMessage>
      <img src={willEscaping} height={mediaHeight} width={mediaWidth}/>
    </DefaultInspirationMessage>,
    timeout: 4000,
    options: {...defaultInspirationalToastOptions},
  },
  {
    message: <DefaultInspirationMessage>
      <img src={willJumpScare} height={mediaHeight} width={mediaWidth}/>
    </DefaultInspirationMessage>,
    timeout: 4000,
    options: {...defaultInspirationalToastOptions},
  },
  {
    message: <DefaultInspirationMessage>
          <span className="not-italic font-mono">
            {`⚠️ Orin shutting down in 10 seconds... ⚠️`}
          </span>
    </DefaultInspirationMessage>,
    timeout: 1000,
    options: {...defaultInspirationalToastOptions},
    repeat: false,
  }
]