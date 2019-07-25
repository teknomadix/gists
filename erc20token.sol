pragam solidity ^0.5.1;

contract ERC20Token {
    
    mapping ( address => mapping ( address => unit256 )) public allowances;
    mapping ( address => unit256) public _balances;
    
    unit256 public totalSupply;
    string public name;
    string public symbol;
    unit8 public decimals;
    
    constructor(unit256 _totalSupply, string memory _name, string memory _symbol, unit8 _decimals) {
        _balances[msg.sender] = _totalSupply;
        name = _name;
        symbol = _symbol;
        decimals = _decimals;
    }


    function balanceOf(address _owner) public view returns(unit256 balance) {
        return balance[_owner];
    }


    function transfer(address _to, unit256 _value) public returns(bool success) {
        // prevents overspending

        require (balances[msg.sender] >= _value);

        // update balances
        balances[msg.sender] -= _value;
        balances[_to] += _value;

        return true;
        
    }

    function transferFrom(address _from, address _to, uint256 _value) public returns (bool success) {
     
            // get allowance
            unit256 allowance = allowances[_from][msg.sender];
            
            // require that value be < balances and allowance
            require (balances[_from] >= _value && allowance >= _value);
        
            // update balances
            balances[_from] -= _value;
            balances[_to] += _value;
        
            // check that allowance < MAX_UINT256
            if(allowance < MAX_UINT256)  {
                
                // deduct from allowance
                allowances[_from][msg.sender] -= _value;
            }
                
            return true;
        }
    
    function approve(address _spender, uint256 _value) public returns (bool success){
        allowances[msg.sender][_spender] = _value;
        return true;

    }

    function allowance(address _owner, address _spender) public view returns (uint256 remaining){
        return allowances[_owner][_spender];
    }


}
