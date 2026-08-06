import { Card, CardBody, CardHeader, CardProps, Switch } from "@nextui-org/react";
import { useEffect, useRef, useState } from "react";
import { Info, Lock } from "react-feather";
import { RootState } from "../../../redux/RootState";
import { useSelector } from "react-redux";

import * as THREE from "three";
import URDFLoader, { URDFRobot } from "urdf-loader";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls.js";

const EMPTY_PACKAGES: Record<string, string> = {};

// Properties for the URDFWidget component.
export interface IURDFWidgetProps extends CardProps {
  /** Path to the .urdf file, relative to public/ or an absolute URL */
  urdfPath: string;
  /** package:// name -> resolved path mapping, mirrors URDFLoader.packages */
  packages?: Record<string, string>;
}

const COLLISION_MATERIAL = new THREE.MeshBasicMaterial({
  color: 0xff3333,
  wireframe: true,
});

/**
 * A component that renders a live 3D URDF model of the rover, with an
 * optional overlay of the collision geometry from <collision> nodes.
 */
const URDFWidget: React.FC<IURDFWidgetProps> = ({
  urdfPath,
  packages = EMPTY_PACKAGES,
  ...props
}) => {
  const isConnected = useSelector((state: RootState) => state.driveStore.connected);

  const [widgetLocked, setWidgetLock] = useState<boolean>(true);
  const [showCollisions, setShowCollisions] = useState<boolean>(false);

  // ref so the load callback (which fires async, possibly after several
  // toggle clicks) always applies the current visibility, not a stale one
  const showCollisionsRef = useRef(showCollisions);
  useEffect(() => {
    showCollisionsRef.current = showCollisions;
    applyCollisionVisibility(robotRef.current, showCollisions);
  }, [showCollisions]);

  // Blur put over the viewport when widget is disabled
  const blurOverlay = (
    <div className="URDFWidgetOverlay flex flex-col justify-center content-center backdrop-blur-[1px] wrap-none" />
  );
  const widgetLockMessage = (
    <div className="flex flex-row justify-center gap-2">
      <Lock /> <span>Widget is disabled</span>
    </div>
  );
  const disconnectedMessage = (
    <div className="flex flex-row justify-center gap-2">
      <Info /> <span>Disconnected</span>
    </div>
  );

  const mountRef = useRef<HTMLDivElement>(null);
  const robotRef = useRef<URDFRobot | null>(null);

  // set up the three.js scene + load the URDF once
  useEffect(() => {
    const mount = mountRef.current;
    if (!mount || !urdfPath) return;

    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0x1a1a1a);

    const camera = new THREE.PerspectiveCamera(
      50,
      mount.clientWidth / mount.clientHeight,
      0.01,
      100
    );
    camera.position.set(1.5, 1.2, 1.5);

    const renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setSize(mount.clientWidth, mount.clientHeight);
    mount.appendChild(renderer.domElement);

    const controls = new OrbitControls(camera, renderer.domElement);
    controls.target.set(0, 0.2, 0);
    controls.update();

    scene.add(new THREE.HemisphereLight(0xffffff, 0x444444, 1.2));
    const dirLight = new THREE.DirectionalLight(0xffffff, 1);
    dirLight.position.set(2, 4, 2);
    scene.add(dirLight);

    scene.add(new THREE.GridHelper(4, 20));

    const loader = new URDFLoader();
    loader.packages = packages;
    loader.parseCollision = true; // populate robot.colliders from <collision> nodes

    loader.load(urdfPath, robot => {
      robot.rotation.x = -Math.PI / 2; // URDF is Z-up, three.js is Y-up
      scene.add(robot);
      robotRef.current = robot;
      applyCollisionVisibility(robot, showCollisionsRef.current);
    });

    let frameId: number;
    const animate = () => {
      controls.update();
      renderer.render(scene, camera);
      frameId = requestAnimationFrame(animate);
    };
    animate();

    const handleResize = () => {
      camera.aspect = mount.clientWidth / mount.clientHeight;
      camera.updateProjectionMatrix();
      renderer.setSize(mount.clientWidth, mount.clientHeight);
    };
    window.addEventListener("resize", handleResize);

    return () => {
      cancelAnimationFrame(frameId);
      window.removeEventListener("resize", handleResize);
      controls.dispose();
      renderer.dispose();
      mount.removeChild(renderer.domElement);
      if (robotRef.current) disposeObject3D(robotRef.current);
      robotRef.current = null;
    };
  }, [urdfPath, packages]);

  return (
    <Card className="no-scroll" {...props}>
      <CardHeader className="gap-3">
        <span>URDF Viewer</span>
        <Switch
          size="sm"
          isSelected={!widgetLocked}
          onChange={() => setWidgetLock(!widgetLocked)}
        />
        <span className="text-sm">Collisions</span>
        <Switch
          size="sm"
          isSelected={showCollisions}
          onChange={() => setShowCollisions(!showCollisions)}
        />
        <div className="grow" />
        {widgetLocked ? widgetLockMessage : !isConnected ? disconnectedMessage : <></>}
      </CardHeader>
      <CardBody className="relative p-3">
        <div ref={mountRef} className="w-full h-full min-h-[300px]" />
        {widgetLocked ? blurOverlay : <></>}
      </CardBody>
    </Card>
  );
};

function applyCollisionVisibility(robot: URDFRobot | null, visible: boolean) {
  if (!robot?.colliders) return;
  Object.values(robot.colliders).forEach(collider => {
    collider.traverse(child => {
      const mesh = child as THREE.Mesh;
      if ((mesh as any).isMesh) {
        mesh.material = COLLISION_MATERIAL;
        mesh.visible = visible;
      }
    });
  });
}

function disposeObject3D(obj: THREE.Object3D) {
  obj.traverse(child => {
    const mesh = child as THREE.Mesh;
    if (mesh.geometry) mesh.geometry.dispose();
    if (mesh.material) {
      const materials = Array.isArray(mesh.material) ? mesh.material : [mesh.material];
      materials.forEach(m => m.dispose());
    }
  });
}

export default URDFWidget;