const fs = require('fs');
const Web3 = require('web3');
const web3 = new Web3('http://localhost:7545');
const web3_ = new Web3('ws://localhost:7545');
const divider = '------------------------------------------------------------------';
async function initAccount(_privateKey) {
    const myAccount = await web3.eth.accounts.privateKeyToAccount(_privateKey);
    return myAccount.address;
}
async function initContract(_web3, _abi, _contractAddress) {
    const jsonInterface = JSON.parse(fs.readFileSync(_abi, 'utf8'));
    return new _web3.eth.Contract(jsonInterface, _contractAddress);
}

async function listenNewTask(_contract, _address, _managePath) {
    console.log(`@[${new Date().toJSON()}] Chair person begins to listen to new tasks..\n`);
    _contract.events.EventInformTaskInfo((error, event) => {
        const Result = event.returnValues;
        console.log(`\n${divider}\n@[${new Date().toJSON()}] EventInformTaskInfo received, operating file: ${Result[2]}`);
        console.log(`The new task's address is ${Result[0]}\n`);
        fs.writeFileSync(_managePath, `[@${new Date().toJSON()}] ${Result[0]}\n`, { flag: 'a', encoding: 'utf8' });
        scheduleAudit(Result[0], _address);
    });
}

async function scheduleAudit(_contractAddr, _address) {
    const contractT1 = await initContract(web3, abiPath2, _contractAddr); // T1 for http while T2 for ws
    const contractT2 = await initContract(web3_, abiPath2, _contractAddr);
    const param = [];
    const option = { from: _address }
    const informSubmitHashKey1 = setTimeout(() => {
        console.log(`@[${new Date().toJSON()}] Informing submit hash key 1...`);
        contractT1.methods.informSubmitHashKey1(...param).estimateGas(option, (error, gas) => {
            console.log(`The informSubmitHashKey1's estimate gas is`, gas || 0);
            contractT1.methods.informSubmitHashKey1(...param)
                .send({ ...option, gas }, (error, txHash) => { console.log(error || txHash, '\n'); });
        });
    }, 10000);
    const informProofGen = setTimeout(() => {
        console.log(`@[${new Date().toJSON()}] Informing proof generation...`);
        contractT1.methods.informProofGen(...param).estimateGas(option, (error, gas) => {
            console.log(`The informProofGen's estimate gas is`, gas || 0);
            contractT1.methods.informProofGen(...param)
                .send({ ...option, gas }, (error, txHash) => { console.log(error || txHash, '\n'); });
        });
    }, 20000);
    const informSubmitHashKey2 = setTimeout(() => {
        contractT1.methods.informSubmitHashKey2(...param).estimateGas(option, (error, gas) => {
            console.log(`The informSubmitHashKey2's estimate gas is`, gas || 0);
            contractT1.methods.informSubmitHashKey2(...param)
                .send({ ...option, gas }, (error, txHash) => { console.log(error || txHash); });
        });
    }, 30000);

    const judge = setTimeout(() => {
        contractT1.methods.judge(...param).estimateGas(option, (error, gas) => {
            console.log(`The judge's estimate gas is`, gas || 0);
            contractT1.methods.judge(...param)
                .send({ ...option, gas }, (error, txHash) => { console.log(error || txHash); });
        });
    }, 40000);

    contractT2.events.EventInformTaskDone((error, event) => {
        console.log(`@[${new Date().toJSON()}] EventInformTaskDone received`);
        clearTimeout(informSubmitHashKey1);
        clearTimeout(informProofGen);
        clearTimeout(informSubmitHashKey2);
        clearTimeout(judge);
        contractT1.methods.withdraw(...param).estimateGas(option, (error, gas) => {
            console.log(`The withdraw's estimate gas is`, gas || 0);
            contractT1.methods.withdraw(...param)
                .send({ ...option, gas }, (error, txHash) => { console.log(error || txHash); });
        });
    });
}
const config = require('../config');
const privateKey = config.chPrivateKey;
const contractAddress = config.contractAddress;
const abiPath = config.abiPath;
const abiPath2 = config.abiPath2;
const bcPath = config.bcPath;
async function main() {
    const address = await initAccount(privateKey);
    const contract = new web3.eth.Contract(JSON.parse(fs.readFileSync(abiPath, 'utf8')))
    const option1 = { data: '0x' + JSON.parse(fs.readFileSync(bcPath, 'utf8')).object }
    const option2 = { from: address }
    contract.deploy(option1).estimateGas(option2, (error, gas) => {
        contract.deploy(option1).send({ ...option2, gas },
            function (error, transactionHash) { })
            .on('receipt', async function (receipt) {
                const deployedAddr = receipt.contractAddress;
                console.log("Management contract address : ", deployedAddr);
                fs.writeFileSync(contractAddress, deployedAddr, 'utf8');
                const contract = await initContract(web3_, abiPath, deployedAddr);
                await listenNewTask(contract, address, `./instance-address/${deployedAddr}.txt`);
            });
    });
}
main();