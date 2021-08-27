const fs = require('fs');
const exec = require('child_process').exec;
const Web3 = require('web3');
const web3 = new Web3('http://localhost:7545');
const divider = '------------------------------------------------------------------';
async function initAccount(_web, _privateKey) {
    const myAccount = await _web.eth.accounts.privateKeyToAccount(_privateKey);
    return myAccount.address;
}
async function initContract(_web, _abi, _contractAddress) {
    const jsonInterface = JSON.parse(fs.readFileSync(_abi, 'utf8'));
    return new _web.eth.Contract(jsonInterface, _contractAddress);
}
async function enrollFile(_address, _contract, _srvcPrvdr, _fileName, _blkSize, _blkAmnt) {
    const param = [_srvcPrvdr, _fileName, _blkSize, _blkAmnt];
    const option = { from: _address }
    const gas = await _contract.methods.enrollFile(...param).estimateGas(option);
    console.log('The estimate gas is', gas);
    _contract.methods.enrollFile(...param)
        .send({ ...option, gas }, (error, txHash) => { console.log(error || txHash); });
}
async function requestAudit(_address, _contract, _fileName, _chalAmnt, _contractAddress) {
    const web3_ = new Web3('ws://localhost:7545');
    console.log(`@[${new Date().toJSON()}] Date owner begins to listen to own audit tasks..`);
    const contract = await initContract(web3_, abiPath, _contractAddress);
    contract.events.EventInformTaskInfo(async function (error, event) {
        const Result = event.returnValues;
        const _contractAddr = Result[0];
        console.log(`\n${divider}\n@[${new Date().toJSON()}] EventInformTaskInfo received and the new contract address: ${_contractAddr}`);
        if (Result[1] == _address) {
            console.log(`@[${new Date().toJSON()}] It's my file, begin to listenAudit...\n`);
            const contractT1 = await initContract(web3, abiPath2, _contractAddr); // T1 for http while T2 for ws
            const contractT2 = await initContract(web3_, abiPath2, _contractAddr);
            contractT2.events.EventInformTaskDone((error, event) => {
                console.log(`@[${new Date().toJSON()}] EventInformTaskDone received`);
                const Result = event.returnValues;
                printConclusion(Result[0]);
                process.exit(0);
            });
            contractT2.events.EventInformArbitrate((error, event) => {
                console.log(`@[${new Date().toJSON()}] EventInformArbitrate received, operating file: ${Result[2]}`);
                const _Result = event.returnValues;
                fs.mkdirSync(`../data/${_contractAddr}`);
                fs.writeFileSync(`../data/${_contractAddr}/sum_r`, _Result[0], 'utf8');
                fs.writeFileSync(`../data/${_contractAddr}/sum_s`, _Result[1], 'utf8');
                fs.writeFileSync(`../data/${_contractAddr}/mu`, Buffer.from(web3.utils.hexToBytes(_Result[2])));
                fs.writeFileSync(`../data/${_contractAddr}/sigma`, Buffer.from(web3.utils.hexToBytes(_Result[3])));
                exec(`cd .. && ./arbitrate.out ${_fileName} ${_chalAmnt} ${_contractAddr}`, (error, stdout, stderr) => {
                    console.log(error || stdout || stderr);
                    const rightP = fs.readFileSync(`../data/${_contractAddr}/right_equation`, 'utf8');
                    const result = fs.readFileSync(`../data/${_contractAddr}/result`, 'utf8');
                    const param = [rightP, result];
                    const option = { from: _address }
                    contractT1.methods.arbitrate(...param).estimateGas(option, (error, gas) => {
                        console.log(`The arbitrate's estimate gas is`, gas || 0);
                        contractT1.methods.arbitrate(...param)
                            .send({ ...option, gas }, (error, txHash) => { console.log(error || txHash, '\n'); });
                    });
                });
            });
        } else {
            console.log(`That's not my file.`);
        }
    });

    const param = [_fileName, _chalAmnt];
    const option = { from: _address, value: '1000000000000000000' }
    _contract.methods.requestAudit(...param).estimateGas(option, (error, gas) => {
        console.log(`The requestAudit's estimate gas is `, gas || 0);
        _contract.methods.requestAudit(...param).send({ ...option, gas }, (error, txHash) => {
            console.log(error || txHash, '\n');

        });
    });
}
const config = require('../../config');
const privateKey = config.doPrivateKey;
const manageAddress = config.contractAddress;
const abiPath = config.abiPath;
const abiPath2 = config.abiPath2;
async function main() {
    const contractAddress = fs.readFileSync(manageAddress, 'utf8');
    const address = await initAccount(web3, privateKey);
    const contract = await initContract(web3, abiPath, contractAddress);
    const fileName = '511193429203991005750341479911033238418559712921';
    const spAddress = '0x1D9A64c0814a75d1e43c7c6401E30BDfba4E6526'
    if (process.argv[2] == 'enroll') {
        await enrollFile(address, contract, spAddress, fileName, 4096, 2345);
    } else if (process.argv[2] == 'request') {
        await requestAudit(address, contract, fileName, 10, contractAddress);
    }
}
function printConclusion(result) {
    switch (result) {
        case '0': console.log('All auditors got a same audit solution TRUE, normal ends.'); break;
        case '1': console.log('All auditors got a same audit solution FALSE, normal ends.'); break;
        case '2': console.log('No valid auditors applied in the task.'); break;
        case '3': console.log('The service provider did not response to submit the proof.'); break;
        case '4': console.log('The owner arbitrate the task resulted TRUE, because auditors did not draw a same conclusion.'); break;
        case '5': console.log('The owner arbitrate the task resulted FALSE, because auditors did not draw a same conclusion.'); break;
        default: break;
    }
}
main();