program indirectrecursion;

function isodd(n: integer): integer; forward;
function iseven(n: integer): integer;
begin
    if n > 0 then
    begin
        iseven := isodd(n - 1);
        exit;
    end;
    iseven := 1;
end;

function isodd(n: integer): integer;
begin
    if n > 0 then
    begin
        isodd := iseven(n - 1);
        exit;
    end;
    isodd := 0;
end;

begin
    writeln(iseven(11));
    writeln(isodd(11));
end.
