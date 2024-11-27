/*
import { expect, test } from 'vitest'
import { render } from 'vitest-browser-react'
import '../../index.html';

test('TopBar Empty', async () => {
  const app = App();
  console.log(app);
  //await expect(topBar.baseElement.innerText).toBe("")
  await 0
})
*/


import { expect, test } from 'vitest'
import { render } from 'vitest-browser-react'
//import TopBar from '../components/TopBar/TopBar.tsx'
import NovaGui from './NovaGui.tsx'

test('TopBar Empty', async () => {
  const gui = render(<NovaGui/>)
  console.log(gui)
  await 0;
  //await expect.element(getByText('Hello Vitest!')).toBeInTheDocument()
})
