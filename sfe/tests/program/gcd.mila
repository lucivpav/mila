program gcd;

function gcdi(a: integer; b: integer): integer;
var tmp: integer;
begin
    while b <> 0 do
    begin
        tmp := b;
        b := a mod b;
        a := tmp;
    end;
    gcdi := a;
end;

function gcdr(a: integer; b: integer): integer;
var tmp: integer;
begin
    tmp := a mod b;
    if tmp = 0 then
    begin
        gcdr := b;
        exit;
    end;
    gcdr := gcdr(b, tmp);
end;

function gcdr_guessing_inner(a: integer; b: integer; c: integer): integer;
begin
    if ((a mod c) = 0) and ((b mod c) = 0) then
    begin
        gcdr_guessing_inner := c;
        exit;
    end;
    gcdr_guessing_inner := gcdr_guessing_inner(a, b, c - 1);
end;

function gcdr_guessing(a: integer; b: integer): integer;
begin
    gcdr_guessing := gcdr_guessing_inner(a, b, b);
end;

begin
    writeln(gcdi(27*2, 27*3));
    writeln(gcdr(27*2, 27*3));
    writeln(gcdr_guessing(27*2, 27*3));
end.
