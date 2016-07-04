program arrayAverage;

var I, TEMP, NUM, SUM : integer;
var X : array [-20 .. 20] of integer;
begin
  for I := -20 to 20 do begin
    X[I] := 0;
  end;

  readln(NUM);
  
  for I := 0 to NUM - 1 do begin
    readln(TEMP);
    X[TEMP] := X[TEMP] + 1;
  end;

  SUM := 0;
  for I := 20 downto -20 do begin
    SUM := SUM + I * X[I];
  end;
  writeln(SUM div NUM);
end.
