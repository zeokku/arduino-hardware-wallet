import { SerialPort } from "serialport";
import Crypto from "crypto";

import { uint16ToBytes } from "./utils.js";
import { OP_RESULT } from "./error_codes.js";

const port = new SerialPort(
  {
    path: "COM5",
    baudRate: 9600,
  },
  (err) => {
    if (err) console.log(err);
    else console.log("open");
  }
);

port.on("error", (err) => {
  console.log("Error: ", err.message);
});

//so this shit is not called again if not all data is read bruuu
// port.on("readable", () => {
//   //   let output = port.read() as Buffer;
//   //   console.log(output);

//   //THIS SHIT FAILED BECAUSE OF x00 byte apparently bruuuuuuuuhhhhhhh
//   //wait no why
//   //it's only strings anyway????
//   //nah anyway it's prolly some weird ass char that fucks up the string bruh
//   //ok this is utter bullshit
//   //console.log(new TextDecoder().decode(output));

//   //   console.log(output.toString("hex"));
//   //   return;

//   let operation = (port.read(1) as Buffer)[0];

//   console.log("op", operation.toString(16));

//   switch (operation) {
//     case 0x01:
//       {
//         console.log("ready");
//         port.write(Buffer.from([0x20, 0x21]));
//       }
//       break;
//     case 0x10:
//       {
//         let output: Buffer = port.read();

//         let enc = output.slice(0, 32);
//         let input = output.slice(32, 32 + 32);
//         let pass = output.slice(32 + 32, 32 + 32 + 256);

//         console.log(output.length, 32 + 32 + 256);

//         console.log(enc.toString("hex"));
//         console.log(input.toString("hex"));
//         console.log(pass.toString("hex"));
//       }
//       break;
//     default:
//       {
//         let output = port.read();
//         //console.log(output);
//       }
//       break;
//   }
// });

port.on("data", (data: Buffer) => {
  /*
this is guaranteed to have at least one byte
we can create a buffer to concat input data based on this first byte
the first byte is operation ID which would define how many more information we should receive
*/

  let opCode = String.fromCharCode(data[0]);
  let opResult = data[1];

  if (opResult) {
    console.log(`${opCode} operation failed!`);
    console.log(OP_RESULT[opResult]);
    return;
  }

  switch (opCode) {
    case "g":
      {
        console.log("Public key:");
        console.log(data.slice(2).toString("hex"));
      }
      break;
    case "s":
      {
        console.log("Signature success:");
        console.log(data.slice(2).toString("hex"));
      }
      break;
    case "k":
      {
        console.log("Key stored successfully");
      }
      break;
    case "p":
      {
        console.log("Public key:");
        console.log(data.slice(2).toString("hex"));
      }
      break;
    case "x":
      {
        console.log("Private key:");
        console.log(data.slice(2).toString("hex"));
      }
      break;
    default:
      {
        console.log("NOT AN OPERATION");
        console.log(new TextDecoder().decode(data));
        console.log(data.length);
      }
      break;
  }
});

//@todo how to clear password buffers from memory?

// default encoding for Stream.write(string) is 'utf8'

function generate_key(password: string) {
  port.write("g");

  port.write(password);
  port.write(Buffer.from([0]));
  // \0 is optional when it's only password
}

function sign_payload(password: string, payload: Buffer) {
  if (payload.length > 0xffff) {
    console.log("Invalid payload length");
  }

  port.write("s");

  port.write(password);
  port.write(Buffer.from([0]));

  port.write(Buffer.from(uint16ToBytes(payload.length)));

  port.write(payload);
}

function store_key(password: string, private_key: Buffer) {
  port.write("k");

  port.write(password);
  port.write(Buffer.from([0]));

  port.write(private_key);
}

function export_public_key(password: string) {
  port.write("p");

  port.write(password);
  port.write(Buffer.from([0]));
}

function export_private_key(password: string) {
  port.write("x");

  port.write(password);
  port.write(Buffer.from([0]));
}

//pub: 687899739d47f2f699dcd5f752cc0dacff90fc7ff1db55150ce288605354768b
const pwd = "😳👉👈";
//generate_key(pwd);

//sig: 82c299ef66981464dc09c17d57d9f6beedc8821a41655abcef90d6cf006d9df2416d17627f1242007dc4b673ff79333f030e24f58a99f41d8e97e3820683eb01
sign_payload(pwd, Buffer.from("aboba"));

//export_public_key("TestPasswordBruh");
//export_private_key("TestPasswordBruh");

//sign_payload("TestPasswordBruh", Buffer.from("Test payload 12345", "utf-8"));

//let payload = "Test payload aboba with new extra data to test";

//port.write("sTestPasswordBruh");
//port.write("sBruh");
//port.write(Buffer.from([0]));
// port.write(Buffer.from(uint16ToBytes(payload.length)));
// port.write(Buffer.from(payload, "utf-8"));

//apparently buffer ignores \0 in strings bruuuu
// port.write(Buffer.from("kTestPasswordBruh"));
// port.write(Buffer.from([0]));
// port.write(Buffer.from(new Uint8Array(32).fill(0x69, 0, 32)));
// port.drain();
