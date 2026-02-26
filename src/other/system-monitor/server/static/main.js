UPDATE_PERIOD = 5000 // seconds between pings 

let can_id_names = null;       // all CAN IDs of the probe responses and their corresponding boards
let expected_can_ids = null    // all probe response CAN IDs expected after a probe request (e.g. all connected boards)
let probe_responses = null;    // all probe responses in actuality

async function run_probe() {
  /*
    send CAN probe frame
  */
  await fetch("/run_probe");
}

async function populate_probe_responses() {
  /*
    get all recorded probe responses (last one for each CAN ID)
  */
  let response = await fetch("/get_probe_responses");
  probe_responses = await response.json();
  // console.log(result);
}

async function populate_can_ids() {
  /*
    get all probe CAN message IDs used on the rover
  */
  let response = await fetch("/get_can_ids");
  can_id_names = await response.json();
  console.log("ALL can IDs:")
  console.log(can_id_names);
}

async function populate_expected_can_ids() {
  /*
    get probe CAN message IDs of devices that should be connected
    e.g. all connected boards
  */
  let response = await fetch("/get_expected_can_ids");
  expected_can_ids = await response.json();
  console.log(expected_can_ids);
}

function init_page_layout() {
  /*
    create page element from expected can IDs
  */
  for ([category, can_id_arr] of Object.entries(expected_can_ids["expected_id_categories"])) {
    let cat_section = document.createElement("section");
    let cat_title = document.createElement("h2");
    cat_title.innerText = category
    cat_section.appendChild(cat_title);

    // div containing all boards in a category
    let cat_boards = document.createElement("div");
    cat_section.appendChild(cat_boards);

    // add all boards to above div
    for (can_id of can_id_arr) {
      let board_div = document.createElement("div")
      board_div.id = `can_board_${can_id}`;

      let board_title = document.createElement("h3")
      board_title.innerText = can_id_names[can_id];
      board_div.appendChild(board_title);

      let board_info = document.createElement("p")
      board_info.innerText = "waiting for response";
      board_div.appendChild(board_info);
      set_board_status_class(board_div, "board_okay");
      
      cat_boards.appendChild(board_div);
    }

    document.getElementById("status_list").appendChild(cat_section);
  }
}

function set_board_status_class(obj, class_name) {
  // only one of these classes can be attached at a time
  status_classes = ["board_good", "board_okay", "board_bad"];
  if (!status_classes.includes(class_name)) throw Error(`Class ${class_name} not standard`);

  for (status_class of status_classes) {
    if (status_class == class_name) {
      if (obj.classList.contains(status_class)) continue;
      obj.classList.add(status_class);
    }
    else {
      if (!obj.classList.contains(status_class)) continue;
      obj.classList.remove(status_class);
    }
  }
}

function update_page_layout() {
  /*
    does the heavy lifting and actually updates page layout to currently known shit
  */
  console.log(probe_responses)  
  for ([category, can_id_arr] of Object.entries(expected_can_ids["expected_id_categories"])) {
    // each can id
    for (can_id of can_id_arr) {
      let board_div = document.getElementById(`can_board_${can_id}`);
      let board_info = board_div.getElementsByTagName('p')[0];

      // if board has been reached by ping
      if (probe_responses[can_id] != undefined) {
        let time_elapsed_ms = Date.now() - new Date(probe_responses[can_id].probe_time);

        if (time_elapsed_ms <= UPDATE_PERIOD*3) {
          board_info.innerText = "working";
          set_board_status_class(board_div, "board_good");
          continue
        }
        board_info.innerText = `unreachable: last ping ${Math.round(time_elapsed_ms/1000)} seconds ago`;
        set_board_status_class(board_div, "board_okay");
        continue

      }
      board_info.innerText = "unreachable";
      set_board_status_class(board_div, "board_bad");
    }
  }
}

async function update_page() {
  /*
    Update page with probe responses, then start a new probe
  */
  await populate_probe_responses();
  update_page_layout();
  run_probe();
}

async function main() {
  console.log("Banksia System Status");
  await populate_expected_can_ids();
  await populate_can_ids();
  init_page_layout();

  await populate_probe_responses();
  await update_page();
  setInterval(update_page, UPDATE_PERIOD);
}

window.onload = main;
