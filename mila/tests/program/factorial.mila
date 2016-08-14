program factorial;

function facti(n : integer) : integer;
begin
    facti := 1;
    while n > 1 do
    begin
        facti := facti * n;
        dec(n);
    end
end;    

function factr(n : integer) : integer;
begin
    if n = 1 then 
        factr := 1
    else
        factr := n * factr(n-1);
end;    

begin
    writeln(facti(5));
    writeln(factr(5));
end.
