// SPDX-License-Identifier: GPL-3.0
pragma solidity ^0.8.4;
import "./AuditTask.sol";

contract TaskManagement {
    uint256 constant AUDIT_INTERVAL = 5 seconds;
    uint256 constant AUDIT_MINIMAL_FEE = 1 ether;
    address public chairperson;
    struct FileInfo {
        address dataOwner;
        address serviceProvider;
        uint64 blockSize;
        uint192 blockAmount;
        uint256 lastAudit;
    }
    mapping(uint256 => FileInfo) public enrolledFiles; // file name => file infomation

    // audit_contract_address, data_owner_address, file_name, service_provider_address, block_size, block_amount, challenge_amount, audit_fee
    event EventInformTaskInfo(
        address,
        address,
        uint256,
        address,
        uint256,
        uint256,
        uint256,
        uint256
    );

    constructor() {
        chairperson = msg.sender;
    }

    function enrollFile(
        address _serviceProvider,
        uint256 _fileName,
        uint64 _blockSize,
        uint192 _blockAmount
    ) public payable {
        enrolledFiles[_fileName].dataOwner = msg.sender;
        enrolledFiles[_fileName].serviceProvider = _serviceProvider;
        enrolledFiles[_fileName].blockSize = _blockSize;
        enrolledFiles[_fileName].blockAmount = _blockAmount;
        enrolledFiles[_fileName].lastAudit = 0;
    }

    function requestAudit(uint256 _fileName, uint256 _challengeAmount)
        public
        payable
    {
        require(msg.value >= AUDIT_MINIMAL_FEE, "AUDIT_FEE_LT_MINIMAL");
        require(
            msg.sender == enrolledFiles[_fileName].dataOwner,
            "FILE OWNER MISMATCH"
        );
        require(
            block.timestamp - enrolledFiles[_fileName].lastAudit >
                AUDIT_INTERVAL,
            "ADUITE INTERVAL INFFICIENT"
        );

        enrolledFiles[_fileName].lastAudit = block.timestamp;
        AuditTask auditTask = new AuditTask(
            chairperson,
            _fileName,
            _challengeAmount,
            enrolledFiles[_fileName].dataOwner,
            enrolledFiles[_fileName].serviceProvider
        );
        address contractAddr = address(auditTask);
        emit EventInformTaskInfo(
            contractAddr,
            msg.sender,
            _fileName,
            enrolledFiles[_fileName].serviceProvider,
            enrolledFiles[_fileName].blockSize,
            enrolledFiles[_fileName].blockAmount,
            enrolledFiles[_fileName].blockAmount < _challengeAmount
                ? enrolledFiles[_fileName].blockAmount
                : _challengeAmount,
            msg.value
        );
        payable(contractAddr).transfer(address(this).balance);
    }
}
