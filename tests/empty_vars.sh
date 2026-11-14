echo $HUSK_UNSET_VAR_XYZ end
echo "$HUSK_UNSET_VAR_XYZ" end
echo a $HUSK_UNSET_VAR_XYZ b
$HUSK_UNSET_VAR_XYZ
$HUSK_UNSET_VAR_XYZ && echo empty-command-succeeds
echo '' quoted-empty-kept
echo after
